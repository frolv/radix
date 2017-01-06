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

#include <untitled/sched.h>
#include <untitled/task.h>

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

	next = list_first_entry(&task_queue, struct task, queue);

	if (current_task) {
		current_task->state = TASK_READY;
		list_ins(&task_queue, &current_task->queue);
	}
	list_del(&next->queue);

	if (preempt)
		switch_to_task(next);
	else
		current_task = next;
}

void sched_add(struct task *t)
{
	list_ins(&task_queue, &t->queue);
}
