/*
 * kernel/sched/sched.c
 * Copyright (C) 2021 Alexei Frolov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <radix/assert.h>
#include <radix/compiler.h>
#include <radix/cpu.h>
#include <radix/event.h>
#include <radix/ipi.h>
#include <radix/irq.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/kthread.h>
#include <radix/limits.h>
#include <radix/mm.h>
#include <radix/sched.h>
#include <radix/smp.h>
#include <radix/spinlock.h>
#include <radix/tasking.h>
#include <radix/time.h>

#include <rlibc/string.h>

#include <stdbool.h>

#include "idle.h"

#define SCHED_PRIO_LEVELS 20
#define SCHED_NUM_RECENT  10
#define PRIO_BOOST_PERIOD (500 * NSEC_PER_MSEC)

#define SCHED "sched: "

DEFINE_PER_CPU(struct task *, current_task) = NULL;

// Priority queues for the scheduler.
static DEFINE_PER_CPU(struct list, prio_queues[SCHED_PRIO_LEVELS]);
static DEFINE_PER_CPU(spinlock_t,
                      queue_locks[SCHED_PRIO_LEVELS]) = {SPINLOCK_INIT};

// List of tasks recently run on this CPU to assist with cache-efficient
// scheduling.
// TODO(frolv): Think about this some more.
static DEFINE_PER_CPU(struct task *, recent_tasks[SCHED_NUM_RECENT]) = {NULL};

static DEFINE_PER_CPU(struct task *, prio_boost_task) = NULL;
static DEFINE_PER_CPU(int, active_tasks) = 0;

static void __prio_boost(void *p);

// Initialize this processor's MLFQ priority boosting task.
static int __priority_boost_task_init(void)
{
    int cpu = processor_id();
    struct task *pb =
        kthread_create(__prio_boost, NULL, 0, "prio_boost_%u", cpu);
    if (IS_ERR(pb)) {
        klog(KLOG_ERROR,
             SCHED "failed to initialize priority boost task for cpu %u",
             cpu);
        return 1;
    }

    pb->cpu_restrict = CPUMASK_SELF;

    this_cpu_write(prio_boost_task, pb);

    // TODO(frolv): The kernel requires sleep() support before a prio boost
    // task can effectively run.
    // kthread_start(pb);

    return 0;
}

int sched_init(void)
{
    this_cpu_write(current_task, NULL);
    this_cpu_write(active_tasks, 0);

    for (int i = 0; i < SCHED_PRIO_LEVELS; ++i) {
        list_init(raw_cpu_ptr(&prio_queues[i]));
        spin_init(raw_cpu_ptr(&queue_locks[i]));
    }

    if (idle_task_init() != 0) {
        return 1;
    }

    if (__priority_boost_task_init() != 0) {
        return 1;
    }

    return 0;
}

// Returns the starting timeslice for a task of a given priority level.
//
// NOTE: if SCHED_PRIO_LEVELS is changed, this should probably be updated.
static __always_inline uint64_t __prio_timeslice(int prio)
{
    return (5 + (prio / 2)) * NSEC_PER_MSEC;
}

// Finds the most suitable CPU on which to run the new task `t`.
static int __find_best_cpu(struct task *t)
{
    int cpu, best, min_tasks, curr_tasks;
    cpumask_t potential;

    potential = cpumask_online() & t->cpu_restrict;
    min_tasks = INT_MAX;
    best = -1;

    for_each_cpu (cpu, potential) {
        curr_tasks = cpu_var(active_tasks, cpu);

        // If the CPU is not running anything, choose it.
        if (!curr_tasks) {
            return cpu;
        }

        if (curr_tasks < min_tasks) {
            min_tasks = curr_tasks;
            best = cpu;
        }
    }

    return best;
}

void __add_task_to_cpu(struct task *task, int cpu)
{
    unsigned long irqstate;
    spinlock_t *lock = cpu_ptr(&queue_locks[task->prio_level], cpu);

    spin_lock_irq(lock, &irqstate);
    list_ins(cpu_ptr(&prio_queues[task->prio_level], cpu), &task->queue);
    spin_unlock_irq(lock, irqstate);

    atomic_inc(cpu_ptr(&active_tasks, cpu));

    if (is_idle(cpu)) {
        send_sched_wake(cpu);
    }
}

int sched_add(struct task *t)
{
    int cpu = __find_best_cpu(t);
    if (cpu == -1) {
        return 1;
    }

    t->cpu_affinity = 0;
    t->prio_level = 0;
    t->sched_ts = 0;
    t->state = TASK_READY;
    t->remaining_time = __prio_timeslice(t->prio_level);

    __add_task_to_cpu(t, cpu);
    return 0;
}

static struct task *__select_next_task(void)
{
    unsigned long irqstate;

    // Find the highest priority task available and choose that.
    for (int prio = 0; prio < SCHED_PRIO_LEVELS; ++prio) {
        struct list *queue = raw_cpu_ptr(&prio_queues[prio]);
        spinlock_t *lock = raw_cpu_ptr(&queue_locks[prio]);

        spin_lock_irq(lock, &irqstate);
        if (!list_empty(queue)) {
            struct task *t = list_first_entry(queue, struct task, queue);
            list_del(&t->queue);
            spin_unlock_irq(lock, irqstate);
            return t;
        }
        spin_unlock_irq(lock, irqstate);
    }

    return NULL;
}

// Adds the specified task to this CPU's list of recently run tasks.
static void __update_recent_tasks(struct task *task)
{
    struct task **recent, *evict;

    recent = this_cpu_ptr(&recent_tasks[0]);
    evict = recent[SCHED_NUM_RECENT - 1];

    // Check to see if the evicted task is no longer in the list. If so,
    // remove its affinity to this CPU.
    if (evict && evict != task) {
        for (int i = 0; i < SCHED_NUM_RECENT - 1; ++i) {
            if (recent[i] == evict) {
                goto shift_tasks;
            }
        }
        evict->cpu_affinity &= ~CPUMASK_SELF;
    }

shift_tasks:
    memmove(recent + 1, recent, sizeof *recent * (SCHED_NUM_RECENT - 1));
    recent[0] = task;
}

// Updates the remaining time of a task based on the current timestamp.
// Returns true if the task's timeslice has expired.
static bool __update_task_timeslice(struct task *task, uint64_t sched_ts)
{
    uint64_t elapsed = sched_ts - task->sched_ts;
    if (elapsed + MIN_EVENT_DELTA >= task->remaining_time) {
        // The task has used up its allotted time at this priority.
        // Move it down to the next level.
        task->prio_level = min(task->prio_level + 1, SCHED_PRIO_LEVELS - 1);
        task->remaining_time = __prio_timeslice(task->prio_level);
        return true;
    }

    task->remaining_time -= elapsed;
    return false;
}

// Processes a task that is being replaced by putting in into a scheduler queue,
// if applicable.
static void __handle_outgoing_task(struct task *outgoing)
{
    if (outgoing == NULL || outgoing == this_cpu_read(idle_task)) {
        // Nothing to do.
        return;
    }

    spinlock_t *lock;

    if (outgoing->state != TASK_RUNNING) {
        atomic_dec(raw_cpu_ptr(&active_tasks));
    }

    if (outgoing->state == TASK_FINISHED) {
        // TODO(frolv): Temporarily free finished tasks immediately. This should
        // be deferred to allow the parent to consume the result of the task.
        task_free(outgoing);
        return;
    }

    __update_recent_tasks(outgoing);

    if (outgoing->state != TASK_BLOCKED) {
        lock = raw_cpu_ptr(&queue_locks[outgoing->prio_level]);
        spin_lock(lock);
        list_ins(raw_cpu_ptr(&prio_queues[outgoing->prio_level]),
                 &outgoing->queue);
        outgoing->state = TASK_READY;
        spin_unlock(lock);
    }
}

static void __prepare_next_task(struct task *next, uint64_t now)
{
    next->state = TASK_RUNNING;
    next->cpu_affinity |= CPUMASK_SELF;
    next->sched_ts = now;

    cpu_set_kernel_stack(next->stack_top);
    this_cpu_write(current_task, next);

    switch_address_space(next->vmm);

    // TODO(frolv): Figure out how to handle failed sched event insertions.
    int err = sched_event_add(now + next->remaining_time);
    if (err != 0) {
        panic("could not create scheduler event for cpu %u: %s\n",
              processor_id(),
              strerror(err));
    }
}

void schedule(enum sched_action action)
{
    assert(action == SCHED_SELECT || action == SCHED_REPLACE);

    if (action == SCHED_REPLACE && in_irq()) {
        panic("Doesn't make sense to replace from within an interrupt");
    }

    uint64_t sched_ts = time_ns();
    struct task *curr = current_task();

    if (curr) {
        __update_task_timeslice(curr, sched_ts);
    }

    __handle_outgoing_task(curr);

    if (action == SCHED_REPLACE) {
        sched_event_del();
    }

    struct task *next = __select_next_task();
    if (!next) {
        next = this_cpu_read(idle_task);
    }

    __prepare_next_task(next, sched_ts);
    set_cpu_active(processor_id());

    if (action == SCHED_REPLACE && curr != next) {
        switch_task(curr, next);
    }
}

// Iterate over all tasks in the specified queue, boosting the priority
// of those which have not run in a sufficiently long period.
static __always_inline void __prio_boost_queue(struct list *queue,
                                               spinlock_t *lock,
                                               uint64_t now)
{
    struct list *l, *tmp;
    struct task *t;
    unsigned long irqstate;

    if (list_empty(queue)) {
        return;
    }

    struct list *max_prio = raw_cpu_ptr(&prio_queues[0]);
    spinlock_t *max_lock = raw_cpu_ptr(&queue_locks[0]);

    spin_lock_irq(lock, &irqstate);
    list_for_each_safe (l, tmp, queue) {
        t = list_entry(l, struct task, queue);
        if (t->sched_ts == 0 || now - t->sched_ts < PRIO_BOOST_PERIOD) {
            continue;
        }

        list_del(&t->queue);
        t->prio_level = 0;
        t->remaining_time = __prio_timeslice(0);

        spin_lock(max_lock);
        list_ins(max_prio, &t->queue);
        spin_unlock(max_lock);
    }
    spin_unlock_irq(lock, irqstate);
}

static __noreturn void __prio_boost(__unused void *p)
{
    while (1) {
        struct task *this = current_task();
        uint64_t now = time_ns();

        for (int prio = 1; prio < SCHED_PRIO_LEVELS; ++prio) {
            __prio_boost_queue(raw_cpu_ptr(&prio_queues[prio]),
                               raw_cpu_ptr(&queue_locks[prio]),
                               now);
        }

        // Reset the boost task's timeslice so that its own prio_level
        // is never dropped.
        this->prio_level = 0;
        this->remaining_time = __prio_timeslice(0);

        // TODO(frolv): Replace this with a sleep.
        schedule(SCHED_REPLACE);
    }
}

void sched_unblock(struct task *task)
{
    assert(task != NULL);
    assert(task->state == TASK_BLOCKED);

    // TODO(frolv): Temporary code so that mutexes work.
    int cpu = __find_best_cpu(task);
    if (cpu == -1) {
        panic("Could not find CPU to unblock task %s", task->cmdline[0]);
    }

    task->state = TASK_READY;
    __add_task_to_cpu(task, cpu);
}
