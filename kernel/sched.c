/*
 * kernel/sched.c
 * Copyright (C) 2016-2017 Alexei Frolov
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

#include <radix/irq.h>
#include <radix/sched.h>
#include <radix/task.h>

struct task *current_task = NULL;

/* For temporary basic RR scheduler. */
static struct list task_queue;

static int sched_active = 0;

void sched_init(void)
{
	if (!sched_active) {
		list_init(&task_queue);
		sched_active = 1;
	}
}

/*
 * schedule: select a task to run.
 * If preempt is 1, the current running task is preempted
 * and replaced with the new scheduled task.
 */
void schedule(int preempt)
{
	struct task *next;

	if (unlikely(list_empty(&task_queue)))
		return;

	/*
	 * Don't allow current task to be preempted by another
	 * source while it is yielding the CPU.
	 */
	if (preempt)
		irq_disable();

	next = list_first_entry(&task_queue, struct task, queue);

	/*
	 * The current task may be blocked, in which case it should not
	 * be added to the ready queue.
	 */
	if (current_task && current_task->state == TASK_RUNNING) {
		current_task->state = TASK_READY;
		list_ins(&task_queue, &current_task->queue);
	}
	list_del(&next->queue);

	if (preempt) {
		switch_to_task(next);
		irq_enable();
	} else {
		current_task = next;
		current_task->state = TASK_RUNNING;
	}
}

void sched_add(struct task *t)
{
	t->state = TASK_READY;
	list_ins(&task_queue, &t->queue);
}

/*
 * sched_unblock:
 * Called when a resource held by `t` becomes available.
 * Scheduler decides whether to preempt the current thread and run `t`
 * or to add `t` to the queue of waiting threads.
 */
void sched_unblock(struct task *t)
{
	/* TODO: since we don't have priorities yet, just add to wait queue */
	t->state = TASK_READY;
	list_ins(&task_queue, &t->queue);
}
