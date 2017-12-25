/*
 * kernel/task.c
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

#include <radix/error.h>
#include <radix/kernel.h>
#include <radix/kthread.h>
#include <radix/sched.h>
#include <radix/slab.h>
#include <radix/tasking.h>

#include <rlibc/string.h>

static struct slab_cache *task_cache;

static void task_init(void *t);

void tasking_init(void)
{
	struct task *curr;

	task_cache = create_cache("task_cache", sizeof (struct task),
	                          SLAB_MIN_ALIGN,
	                          SLAB_HW_CACHE_ALIGN | SLAB_PANIC,
	                          task_init);
	sched_init();

	/*
	 * Create a task stub for the currently running kernel boot and save it
	 * as the current task. When the first context switch occurs, registers
	 * will be saved to turn the stub into a proper task.
	 */
	curr = alloc_cache(task_cache);
	if (IS_ERR(curr)) {
		panic("failed to allocate task for main kernel thread: %s\n",
		      strerror(ERR_VAL(curr)));
	}
	curr->state = TASK_RUNNING;
	curr->cmdline = kmalloc(sizeof (*curr->cmdline) << 1);
	curr->cmdline[0] = kmalloc(KTHREAD_NAME_LEN);
	strcpy(curr->cmdline[0], "kernel_boot_thread");
	curr->cmdline[1] = NULL;

	this_cpu_write(current_task, curr);
}

/* Allocate and initialize a new task struct for a kthread. */
struct task *kthread_task(void)
{
	return alloc_cache(task_cache);
}

void task_free(struct task *task)
{
	free_cache(task_cache, task);
}

static void task_init(void *t)
{
	struct task *task;

	task = t;
	memset(task, 0, sizeof *task);
	list_init(&task->queue);
}
