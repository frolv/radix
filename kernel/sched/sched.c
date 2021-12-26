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
#include <radix/sleep.h>
#include <radix/smp.h>
#include <radix/spinlock.h>
#include <radix/tasking.h>
#include <radix/time.h>

#include <rlibc/string.h>

#include <stdbool.h>

#include "idle.h"

#define SCHED_PRIO_LEVELS    20
#define SCHED_MIN_PRIO_LEVEL (SCHED_PRIO_LEVELS - 1)
#define SCHED_NUM_RECENT     10

#define PRIO_BOOST_PERIOD (500 * NSEC_PER_MSEC)

#define SCHED "sched: "

DEFINE_PER_CPU(struct task *, current_task) = NULL;

// Priority queues for the scheduler.
static DEFINE_PER_CPU(struct list, prio_queues[SCHED_PRIO_LEVELS]);
static DEFINE_PER_CPU(spinlock_t,
                      queue_locks[SCHED_PRIO_LEVELS]) = {SPINLOCK_INIT};

// Queue of tasks which have become unblocked.
static DEFINE_PER_CPU(struct list, unblock_queue);
static DEFINE_PER_CPU(spinlock_t, unblock_queue_lock) = SPINLOCK_INIT;

// List of tasks recently run on this CPU to assist with cache-efficient
// scheduling.
// TODO(frolv): Think about this some more.
static DEFINE_PER_CPU(struct task *, recent_tasks[SCHED_NUM_RECENT]) = {NULL};

static DEFINE_PER_CPU(struct task *, prio_boost_task) = NULL;
static DEFINE_PER_CPU(int, active_tasks) = 0;

static DEFINE_PER_CPU(uint64_t, time_spent_idling) = 0;

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

    kthread_start(pb);
    return 0;
}

int sched_init(void)
{
    this_cpu_write(current_task, NULL);
    this_cpu_write(active_tasks, 0);
    this_cpu_write(time_spent_idling, 0);

    for (int i = 0; i < SCHED_PRIO_LEVELS; ++i) {
        list_init(raw_cpu_ptr(&prio_queues[i]));
        spin_init(raw_cpu_ptr(&queue_locks[i]));
    }

    list_init(raw_cpu_ptr(&unblock_queue));
    spin_init(raw_cpu_ptr(&unblock_queue_lock));

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
static int __find_best_cpu(const struct task *t)
{
    cpumask_t potential = cpumask_online() & t->cpu_restrict;
    int min_tasks = INT_MAX;
    int best = -1;

    int cpu;
    for_each_cpu (cpu, potential) {
        int curr_tasks = cpu_var(active_tasks, cpu);

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

int sched_add(struct task *task)
{
    task->cpu_affinity = 0;
    task->prio_level = 0;
    task->sched_ts = 0;
    task->state = TASK_READY;
    task->remaining_time = __prio_timeslice(task->prio_level);

    int cpu = __find_best_cpu(task);
    if (cpu == -1) {
        return 1;
    }

    atomic_inc(cpu_ptr(&active_tasks, cpu));

    unsigned long irqstate;
    spinlock_t *lock = cpu_ptr(&queue_locks[task->prio_level], cpu);

    spin_lock_irq(lock, &irqstate);
    list_ins(cpu_ptr(&prio_queues[task->prio_level], cpu), &task->queue);
    spin_unlock_irq(lock, irqstate);

    if (is_idle(cpu)) {
        send_sched_wake(cpu);
    }

    return 0;
}

// Inserts a task into the local processor's priority queues.
static void __insert_into_prio_queue(struct task *task)
{
    spinlock_t *lock = raw_cpu_ptr(&queue_locks[task->prio_level]);
    spin_lock(lock);
    list_ins(raw_cpu_ptr(&prio_queues[task->prio_level]), &task->queue);
    task->state = TASK_READY;
    spin_unlock(lock);
}

// Finds the highest priority task in the scheduler's unblock queue, if any
// exist. If `reconsider` is not NULL, compare tasks to it as well. Other tasks
// in the unblock queue are returned to the scheduler's priority queues.
//
// Following this function, the unblock queue will be empty.
static struct task *__select_task_from_unblock_queue(struct task *reconsider)
{
    const struct task *curr = current_task();
    struct task *best = reconsider;

    spinlock_t *lock = raw_cpu_ptr(&unblock_queue_lock);
    struct list *unblock_q = raw_cpu_ptr(&unblock_queue);

    spin_lock(lock);

    // Drain the unblock queue, finding the highest priority task. Add all other
    // tasks back into the scheduler's regular priority queues.
    while (!list_empty(unblock_q)) {
        struct task *task = list_first_entry(unblock_q, struct task, queue);
        list_del(&task->queue);

        if (best == NULL) {
            best = task;
            continue;
        }

        // Due to the nature of SMP, it's possible that the current task has
        // become unblocked by another CPU and ended up in the unblock queue
        // while the scheduler is replacing it. If the current task is seen in
        // the queue, still consider it, but don't add it back to a priority
        // queue if it isn't chosen as it will be added later.
        if (task_comparator(best, task) > 0) {
            best->state = TASK_READY;
            if (best != curr) {
                __insert_into_prio_queue(best);
            }
            best = task;
        } else {
            task->state = TASK_READY;
            if (task != curr) {
                __insert_into_prio_queue(task);
            }
        }
    }

    spin_unlock(lock);
    return best;
}

static struct task *__select_next_task(struct task *reconsider)
{
    // Try to pull a task from the unblock queue, comparing against the
    // reconsidered task, if any.
    struct task *t = __select_task_from_unblock_queue(reconsider);
    if (t != NULL) {
        return t;
    }

    // If no unblocked tasks exist, choose the highest priority task available
    // in the regular priority queues.
    for (int prio = 0; prio < SCHED_PRIO_LEVELS; ++prio) {
        struct list *queue = raw_cpu_ptr(&prio_queues[prio]);
        spinlock_t *lock = raw_cpu_ptr(&queue_locks[prio]);

        spin_lock(lock);
        if (!list_empty(queue)) {
            t = list_first_entry(queue, struct task, queue);
            list_del(&t->queue);
            spin_unlock(lock);
            return t;
        }
        spin_unlock(lock);
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

    if (task->flags & TASK_FLAGS_IDLE) {
        raw_cpu_add(time_spent_idling, elapsed);
        task->remaining_time = __prio_timeslice(SCHED_MIN_PRIO_LEVEL);
        return true;
    }

    if (elapsed + MIN_EVENT_DELTA >= task->remaining_time) {
        // The task has used up its allotted time at this priority.
        // Move it down to the next level.
        task->prio_level = min(task->prio_level + 1, SCHED_MIN_PRIO_LEVEL);
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
    if (outgoing == NULL || (outgoing->flags & TASK_FLAGS_IDLE)) {
        // Nothing to do.
        return;
    }

    if (!task_is_active(outgoing)) {
        atomic_dec(raw_cpu_ptr(&active_tasks));
    }

    if (outgoing->state == TASK_FINISHED) {
        // TODO(frolv): Temporarily free finished tasks immediately. This should
        // be deferred to allow the parent to consume the result of the task.
        task_free(outgoing);
        return;
    }

    __update_recent_tasks(outgoing);

    if (task_is_active(outgoing)) {
        __insert_into_prio_queue(outgoing);
    }
}

static void __prepare_next_task(struct task *next, uint64_t sched_ts)
{
    next->state = TASK_RUNNING;
    next->cpu_affinity |= CPUMASK_SELF;
    next->sched_ts = sched_ts;

    cpu_set_kernel_stack(next->stack_top);
    this_cpu_write(current_task, next);

    switch_address_space(next->vmm);

    // TODO(frolv): Figure out how to handle failed sched event insertions.
    int err = sched_event_add(sched_ts + next->remaining_time);
    if (err != 0) {
        panic("could not create scheduler event for cpu %u: %s\n",
              processor_id(),
              strerror(err));
    }
}

// The main scheduler function. Picks a task to run.
void schedule(enum sched_action action)
{
    assert(action == SCHED_REPLACE || action == SCHED_PREEMPT);

    uint64_t sched_ts = time_ns();
    struct task *curr = current_task();

    bool curr_has_expired = true;
    bool curr_is_schedulable = false;

    if (curr) {
        curr_has_expired = __update_task_timeslice(curr, sched_ts);
        curr_is_schedulable =
            task_is_active(curr) && (curr->flags & TASK_FLAGS_IDLE) == 0;
    }

    // Decide whether to reconsider the current task as a scheduling option.
    // This is done this when the following conditions are met:
    //
    //   1. The task did not voluntarily yield (i.e action is PREEMPT).
    //   2. The task is not blocked or otherwise unschedulable.
    //   3. The task still has time remaining to run.
    //
    // The most common scenario in which these are true is when a blocked task
    // is unblocked and the scheduler must choose whether to preempt the current
    // task in favor of it.
    struct task *reconsider = NULL;
    if (action == SCHED_PREEMPT && curr_is_schedulable && !curr_has_expired) {
        reconsider = curr;
    }

    struct task *next = __select_next_task(reconsider);
    if (!next) {
        // If the current task was not previously reconsidered, but there are no
        // other options, choose it.
        if (curr_is_schedulable) {
            next = curr;
        } else {
            next = this_cpu_read(idle_task);
        }
    }

    if (curr != next) {
        __handle_outgoing_task(curr);
    }

    __prepare_next_task(next, sched_ts);

    set_cpu_active(processor_id());

    if (curr != next) {
        switch_task(curr, next);
    }
}

void sched_unblock(struct task *task)
{
    assert(task != NULL);
    assert(task->state == TASK_BLOCKED);

    // It is possible for a task to become unblocked immediately after it blocks
    // itself. If this happens, it may still be yielding through schedule()
    // while we unblock it, and as another CPU potentially tries to run it. This
    // can result in a mess with multiple processors' schedulers stepping on one
    // another trying to modify the same task.
    //
    // This problem is indicative of a fundamental design flaw in the SMP
    // scheduler; however, it is difficult to solve and may require a large
    // redesign. Certainly, there needs to be a lot more locking around task
    // internals and finer control over what parts of the code modify the task
    // struct, instead of the wild west it is today.
    //
    // For the time being, the hack below seems to work. We should only spin for
    // a short period, as the unblocked task is yielding the processor.
    while (atomic_read(&task->flags) & TASK_FLAGS_ON_CPU) {}

    int cpu = __find_best_cpu(task);
    if (cpu == -1) {
        panic("Could not find CPU to unblock task %s", task->cmdline[0]);
    }

    atomic_inc(cpu_ptr(&active_tasks, cpu));

    unsigned long irqstate;
    spinlock_t *lock = cpu_ptr(&unblock_queue_lock, cpu);

    spin_lock_irq(lock, &irqstate);
    list_ins(cpu_ptr(&unblock_queue, cpu), &task->queue);
    spin_unlock_irq(lock, irqstate);

    send_sched_wake(cpu);
}

// Iterate over all tasks in the specified queue, boosting the priority of those
// which have not run in a sufficiently long period.
static __always_inline void __prio_boost_queue(struct list *queue,
                                               spinlock_t *lock)
{
    struct list *l, *tmp;
    struct task *t;
    unsigned long irqstate;

    if (list_empty(queue)) {
        return;
    }

    struct list *max_prio = raw_cpu_ptr(&prio_queues[0]);
    spinlock_t *max_lock = raw_cpu_ptr(&queue_locks[0]);
    uint64_t now = time_ns();

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

        for (int prio = 1; prio < SCHED_PRIO_LEVELS; ++prio) {
            __prio_boost_queue(raw_cpu_ptr(&prio_queues[prio]),
                               raw_cpu_ptr(&queue_locks[prio]));
        }

        // Reset the boost task's timeslice so that its own prio_level
        // is never dropped.
        this->prio_level = 0;
        this->remaining_time = __prio_timeslice(0);

        sleep(2 * PRIO_BOOST_PERIOD);
    }
}
