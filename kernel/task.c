/*
 * kernel/task.c
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

#include <radix/assert.h>
#include <radix/error.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/kthread.h>
#include <radix/mm.h>
#include <radix/sched.h>
#include <radix/slab.h>
#include <radix/smp.h>
#include <radix/tasking.h>

#include <rlibc/string.h>

static struct slab_cache *task_cache;

static void task_init(void *t);

void tasking_init(void)
{
    task_cache = create_cache("task_cache",
                              sizeof(struct task),
                              SLAB_MIN_ALIGN,
                              SLAB_HW_CACHE_ALIGN | SLAB_PANIC,
                              task_init);
}

void task_exit(struct task *task, int status)
{
    assert(task && task->state == TASK_RUNNING);

    task->state = TASK_FINISHED;
    task->exit_status = status;

    schedule(SCHED_REPLACE);
    __builtin_unreachable();
}

struct task *task_alloc(void) { return alloc_cache(task_cache); }

void task_free(struct task *task)
{
    if (task->stack_top != NULL) {
        uintptr_t stack_base = (uintptr_t)task->stack_top - task->stack_size;
        free_pages(virt_to_page((void *)stack_base));
    }

    if (task->cmdline != NULL) {
        for (char **s = task->cmdline; *s; ++s) {
            kfree(*s);
        }
        kfree(task->cmdline);
    }

    free_cache(task_cache, task);
}

// TODO(frolv): Try something more sophisticated.
static uint32_t next_pid = 1;

static void task_init(void *t)
{
    struct task *task = t;
    memset(task, 0, sizeof *task);

    list_init(&task->queue);
    task->cpu_restrict = CPUMASK_ALL;
    task->pid = atomic_fetch_inc(&next_pid);
}
