/*
 * kernel/sleep.c
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

#include <radix/kernel.h>
#include <radix/sched.h>
#include <radix/sleep.h>

void __sleep_blocking(uint64_t ns)
{
    struct task *curr = current_task();
    unsigned long irqstate;

    // Disable interrupts when going to sleep so the task doesn't get preempted
    // between adding the sleep event and yielding.
    irq_save(irqstate);

    int err = sleep_event_add(curr, time_ns() + ns);
    if (err != 0) {
        // TODO(frolv): Figure out how to deal with an error.
        panic("failed to add sleep event: %d", err);
    }

    curr->state = TASK_BLOCKED;
    schedule(SCHED_REPLACE);

    irq_restore(irqstate);
}
