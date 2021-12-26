/*
 * include/radix/sched.h
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

#ifndef RADIX_SCHED_H
#define RADIX_SCHED_H

#include <radix/irqstate.h>
#include <radix/task.h>

int sched_init(void);

enum sched_action {
    // Chooses a new task to run and replaces the currently executing task with
    // it. After the schedule() call completes, the processor will be running
    // the new task.
    SCHED_REPLACE,

    // Considers replacing the executing task with another, typically from the
    // processor's unblock queue. Depending on the priorities of the tasks,
    // either continues running the current task or switches to the other.
    SCHED_PREEMPT,

};

void schedule(enum sched_action action);

// Yields the current thread to the scheduler.
static inline void sched_yield(void)
{
    unsigned long irqstate;
    irq_save(irqstate);
    schedule(SCHED_REPLACE);
    irq_restore(irqstate);
}

// Adds a task to the scheduler.
int sched_add(struct task *t);

void sched_unblock(struct task *task);

#endif  // RADIX_SCHED_H
