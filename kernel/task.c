/*
 * kernel/task.c
 * Copyright (C) 2016 Alexei Frolov
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

#include <string.h>
#include <untitled/sched.h>
#include <untitled/slab.h>

static struct slab_cache *task_cache;

static void task_init(void *t);

void tasking_init(void)
{
	task_cache = create_cache("task_cache", sizeof (struct task), MIN_ALIGN,
	                          SLAB_HW_CACHE_ALIGN, task_init, NULL);
}

/* Allocate and initialize a new task struct for a kthread. */
struct task *kthread_task(void)
{
	return alloc_cache(task_cache);
}

static void task_init(void *t)
{
	struct task *task;

	task = t;
	memset(task, 0, sizeof *task);
	list_init(&task->queue);
}
