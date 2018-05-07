/*
 * kernel/sched/idle.c
 * Copyright (C) 2017 Alexei Frolov
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

#include <radix/error.h>
#include <radix/kernel.h>
#include <radix/kthread.h>
#include <radix/klog.h>
#include <radix/percpu.h>
#include <radix/smp.h>
#include <radix/task.h>

DEFINE_PER_CPU(struct task *, idle_task);

static void idle_func(void *p)
{
	while (1)
		HALT();

	(void)p;
}

int idle_task_init(void)
{
	struct task *idle;
	int cpu;

	cpu = processor_id();

	idle = kthread_create(idle_func, NULL, 0, "idle_%u", cpu);
	if (IS_ERR(idle)) {
		klog(KLOG_ERROR, "failed to initialize idle task for cpu %u",
		     cpu);
		return 1;
	}

	idle->cpu_restrict = CPUMASK_SELF;
	idle->remaining_time = 0;

	this_cpu_write(idle_task, idle);

	return 0;
}

/* is_idle: check if `cpu` is currently idling */
int is_idle(int cpu)
{
	return cpu_var(current_task, cpu) == cpu_var(idle_task, cpu);
}
