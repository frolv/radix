/*
 * include/radix/task.h
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

#ifndef RADIX_TASK_H
#define RADIX_TASK_H

#include <radix/asm/regs.h>

#include <radix/cpumask.h>
#include <radix/list.h>
#include <radix/mm_types.h>
#include <radix/percpu.h>
#include <radix/types.h>

struct vmm_space;

/*
 * A single task (process/kthread) in the system.
 *
 * Rearranging the members of this struct requires changes
 * to be made to the switch_task function.
 */
struct task {
	int                     state;
	int                     priority;
	int                     exit_status;
	int                     errno;
	pid_t                   pid;
	uid_t                   uid;
	gid_t                   gid;
	mode_t                  umask;
	struct regs             regs;
	struct list             queue;
	struct vmm_space        *vmm;
	void                    *stack_base;
	cpumask_t               cpu_affinity;
	cpumask_t               cpu_restrict;
	uint64_t                sched_ts;
	uint64_t                remaining_time;
	char                    **cmdline;
	char                    *cwd;
	int                     prio_level;
};

enum task_state {
	TASK_STOPPED,
	TASK_READY,
	TASK_BLOCKED,
	TASK_RUNNING,
	TASK_ZOMBIE
};

DECLARE_PER_CPU(struct task *, current_task);
#define current_task() (this_cpu_read(current_task))

#endif /* RADIX_TASK_H */
