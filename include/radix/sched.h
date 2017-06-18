/*
 * include/radix/sched.h
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

#ifndef RADIX_SCHED_H
#define RADIX_SCHED_H

#include <radix/list.h>
#include <radix/percpu.h>
#include <radix/regs.h>
#include <radix/types.h>

/*
 * A single task (process/kthread) in the system.
 *
 * Rearranging the members of this struct requires changes to be made to
 * the switch_to_task function.
 */
struct task {
	int state;
	int priority;
	int exit_code;
	int interrupt_depth;
	pid_t pid;
	uid_t uid;
	gid_t gid;
	mode_t umask;
	struct regs regs;
	struct list queue;
	void *stack_base;
	char **cmdline;
	char *cwd;
};

enum {
	TASK_STOPPED,
	TASK_READY,
	TASK_BLOCKED,
	TASK_RUNNING,
	TASK_ZOMBIE
};

DECLARE_PER_CPU(struct task *, current_task);
#define current_task() (this_cpu_read(current_task))

void schedule(int preempt);

void sched_init(void);

void sched_add(struct task *t);
void sched_del(struct task *t);

void sched_unblock(struct task *t);

#endif /* RADIX_SCHED_H */
