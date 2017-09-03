/*
 * kernel/mutex.c
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

#include <radix/atomic.h>
#include <radix/irq.h>
#include <radix/mutex.h>
#include <radix/sched.h>
#include <radix/tasking.h>

void mutex_init(struct mutex *m)
{
	m->count = 0;
	list_init(&m->queue);
}

/*
 * mutex_lock:
 * Attempt to lock mutex `m`. If it is already locked,
 * put thread into a wait and yield CPU.
 */
void mutex_lock(struct mutex *m)
{
	struct task *curr;
	unsigned long irqstate;

	if (unlikely(in_irq()))
		return;

	while (atomic_swap(&m->count, 1) != 0) {
		/*
		 * TODO: this needs to be spinlocked
		 * when multiprocessing is added
		 */
		irq_save(irqstate);
		curr = current_task();
		curr->state = TASK_BLOCKED;
		list_ins(&m->queue, &curr->queue);
		schedule(1);
		irq_restore(irqstate);
	}
}

/* mutex_unlock: unlock mutex `m` and wake a waiting thread */
void mutex_unlock(struct mutex *m)
{
	struct task *next;
	unsigned long irqstate;

	if (unlikely(in_irq()))
		return;

	m->count = 0;

	/* TODO: this needs to be spinlocked when multiprocessing is added */
	irq_save(irqstate);
	if (!list_empty(&m->queue)) {
		next = list_first_entry(&m->queue, struct task, queue);
		list_del(&next->queue);
		sched_unblock(next);
	}
	irq_restore(irqstate);
}
