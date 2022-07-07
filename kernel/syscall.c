/*
 * kernel/syscall.c
 * Copyright (C) 2022 Alexei Frolov
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

#include <radix/asm/syscall.h>
#include <radix/irqstate.h>
#include <radix/sched.h>
#include <radix/syscall.h>
#include <radix/task.h>

void syscall_init(void) { arch_syscall_init(); }

void sys_exit(int status)
{
    irq_disable();

    struct task *curr = current_task();
    curr->exit_status = status;
    curr->state = TASK_FINISHED;

    schedule(SCHED_REPLACE);
    __builtin_unreachable();
}
