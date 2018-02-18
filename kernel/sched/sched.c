/*
 * kernel/sched/sched.c
 * Copyright (C) 2016-2018 Alexei Frolov
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

#include <radix/compiler.h>
#include <radix/cpu.h>
#include <radix/event.h>
#include <radix/irq.h>
#include <radix/kernel.h>
#include <radix/limits.h>
#include <radix/mm.h>
#include <radix/sched.h>
#include <radix/smp.h>
#include <radix/spinlock.h>
#include <radix/tasking.h>
#include <radix/time.h>

#include <rlibc/string.h>

#include "idle.h"

#define SCHED_PRIO_LEVELS 20
#define SCHED_NUM_RECENT  8

DEFINE_PER_CPU(struct task *, current_task) = NULL;

static DEFINE_PER_CPU(struct list, prio_queues[SCHED_PRIO_LEVELS]);
static DEFINE_PER_CPU(spinlock_t, queue_locks[SCHED_PRIO_LEVELS]) =
	{ SPINLOCK_INIT };
static DEFINE_PER_CPU(struct task *, recent_tasks[SCHED_NUM_RECENT]) = { NULL };

static DEFINE_PER_CPU(int, active_tasks) = 0;

int sched_init(void)
{
	int i;

	for (i = 0; i < SCHED_PRIO_LEVELS; ++i)
		list_init(this_cpu_ptr(&prio_queues[i]));

	if (idle_task_init() != 0)
		return 1;

	return 0;
}

/* XXX: if SCHED_PRIO_LEVELS is changed, this should probably be updated */
static __always_inline uint64_t __prio_timeslice(int prio)
{
	return (5 + (prio / 2)) * MSEC_PER_SEC;
}

/*
 * __find_best_cpu:
 * Find the most suitable CPU on which to run the new task `t`.
 */
static int __find_best_cpu(struct task *t)
{
	int cpu, best, min_tasks, curr_tasks;
	cpumask_t online;

	online = cpumask_online();
	min_tasks = INT_MAX;
	best = -1;

	for_each_cpu(cpu, online) {
		if (!(t->cpu_restrict & CPUMASK_CPU(cpu)))
			continue;

		curr_tasks = cpu_var(active_tasks, cpu);
		if (curr_tasks < min_tasks) {
			min_tasks = curr_tasks;
			best = cpu;
		}
	}

	return best;
}

int sched_add(struct task *t)
{
	int cpu;

	cpu = __find_best_cpu(t);
	if (cpu == -1)
		return 1;

	t->prio_level = 0;
	t->remaining_time = __prio_timeslice(t->prio_level);
	list_ins(cpu_ptr(&prio_queues[t->prio_level], cpu), &t->queue);

	return 0;
}

static struct task *__select_next_task(void)
{
	struct task *t;
	struct list *queue;
	spinlock_t *lock;
	int prio;

	t = NULL;
	for (prio = 0; prio < SCHED_PRIO_LEVELS; ++prio) {
		queue = this_cpu_ptr(&prio_queues[prio]);
		lock = this_cpu_ptr(&queue_locks[prio]);

		spin_lock(lock);
		if (!list_empty(queue)) {
			t = list_first_entry(queue, struct task, queue);
			list_del(&t->queue);
			spin_unlock(lock);
			break;
		}
		spin_unlock(lock);
	}

	return t ? t : this_cpu_read(idle_task);
}

/*
 * __update_recent_tasks:
 * Add the specified task to this CPU's list of recently run tasks.
 */
static void __update_recent_tasks(struct task *new)
{
	struct task **recent, *evict;

	recent = this_cpu_ptr(&recent_tasks[0]);
	evict = recent[SCHED_NUM_RECENT - 1];

	if (evict && evict != new)
		evict->cpu_affinity &= ~CPUMASK_SELF;

	memmove(recent + 1, recent, sizeof *recent * (SCHED_NUM_RECENT - 1));
	recent[0] = new;
}

/*
 * __handle_outgoing_task:
 * Update the priority queue and remaining timeslice of the specified task.
 */
static void __handle_outgoing_task(struct task *outgoing, uint64_t now)
{
	uint64_t elapsed;
	spinlock_t *lock;

	elapsed = now - outgoing->sched_ts;
	if (elapsed + MIN_EVENT_DELTA >= outgoing->remaining_time) {
		/*
		 * The task has used up its alloted time at this
		 * priority, move it down to the next level.
		 */
		outgoing->prio_level = max(outgoing->prio_level - 1,
		                           SCHED_PRIO_LEVELS - 1);
		outgoing->remaining_time =
			__prio_timeslice(outgoing->prio_level);
	} else {
		outgoing->remaining_time -= elapsed;
	}

	__update_recent_tasks(outgoing);

	if (outgoing->state != TASK_BLOCKED) {
		outgoing->state = TASK_READY;

		lock = this_cpu_ptr(&queue_locks[outgoing->prio_level]);
		spin_lock(lock);
		list_ins(this_cpu_ptr(&prio_queues[outgoing->prio_level]),
			 &outgoing->queue);
		spin_unlock(lock);
	}
}

static void __prepare_next_task(struct task *next, uint64_t now)
{
	int err;

	next->state = TASK_RUNNING;
	next->cpu_affinity |= CPUMASK_SELF;
	next->sched_ts = now + MIN_EVENT_DELTA;

	cpu_set_kernel_stack(next->stack_base);
	this_cpu_write(current_task, next);

	switch_address_space(next->vmm);

	/* TODO: figure out how to handle failed sched event insertions */
	if ((err = sched_event_add(now + next->remaining_time)))
		panic("could not create scheduler event for cpu %u: %s\n",
		      processor_id(), strerror(err));
}

void schedule(int preempt)
{
	struct task *curr, *next;
	uint64_t now;

	now = time_ns();
	curr = current_task();

	if (curr && curr != this_cpu_read(idle_task))
		__handle_outgoing_task(curr, now);

	next = __select_next_task();
	__prepare_next_task(next, now);

	if (preempt)
		switch_task(curr, next);
}

void sched_unblock(struct task *t)
{
	(void)t;
}
