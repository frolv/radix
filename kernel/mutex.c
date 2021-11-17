/*
 * kernel/mutex.c
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

#include <radix/atomic.h>
#include <radix/irq.h>
#include <radix/kernel.h>
#include <radix/mutex.h>
#include <radix/sched.h>
#include <radix/tasking.h>

void mutex_init(struct mutex *m)
{
	m->owner = 0;
	spin_init(&m->lock);
	list_init(&m->queue);
}

// Attempts to lock the mutex `m`. If it is already locked, puts the running
// thread into a wait and yields the CPU.
void mutex_lock(struct mutex *m)
{
	struct task *curr = current_task();

	unsigned long irqstate;
	irq_save(irqstate);

	while (1) {
		// Attempt to acquire the mutex. There are two acquisition
		// possibilities: either there is no owner (i.e. owner == 0), or
		// this thread had been waiting for the mutex and the previous
		// owner handed it over by setting this thread as the new owner.
		uintptr_t owner = atomic_cmpxchg(&m->owner, 0, (uintptr_t)curr);
		if (owner == 0 || owner == (uintptr_t)curr) {
			break;
		}

		// Block the task and add it to the mutex's list.
		spin_lock(&m->lock);
		assert(list_empty(&curr->queue));
		curr->state = TASK_BLOCKED;
		list_ins(&m->queue, &curr->queue);
		spin_unlock(&m->lock);

		schedule(1);
	}

	irq_restore(irqstate);
}

// Unlocks mutex `m` and wakes a waiting thread, if any.
// This must be called by the thread that holds the mutex.
void mutex_unlock(struct mutex *m)
{
	struct task *curr = current_task();

	uintptr_t owner = atomic_read(&m->owner);
	assert(owner == (uintptr_t)curr);

	struct task *next = NULL;
	unsigned long irqstate;

	// Hand the mutex over to the first waiter, if one exists, and notify
	// the scheduler.
	spin_lock_irq(&m->lock, &irqstate);
	if (!list_empty(&m->queue)) {
		next = list_first_entry(&m->queue, struct task, queue);
		list_del(&next->queue);
	}
	spin_unlock(&m->lock);

	atomic_swap(&m->owner, (uintptr_t)next);
	irq_restore(irqstate);

	if (next != NULL) {
		sched_unblock(next);
	}
}
