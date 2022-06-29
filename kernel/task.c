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
#include <radix/elf.h>
#include <radix/error.h>
#include <radix/initrd.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/kthread.h>
#include <radix/mm.h>
#include <radix/sched.h>
#include <radix/slab.h>
#include <radix/smp.h>
#include <radix/task.h>
#include <radix/vmm.h>

#include <string.h>

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

static void task_free_cmdline(const struct task *task)
{
    if (task->cmdline != NULL) {
        for (char **s = task->cmdline; *s; ++s) {
            kfree(*s);
        }
        kfree(task->cmdline);
    }
}

void task_free(struct task *task)
{
    if (task->stack_top != NULL) {
        uintptr_t stack_base = (uintptr_t)task->stack_top - task->stack_size;
        free_pages(virt_to_page((void *)stack_base));
    }

    task_free_cmdline(task);

    if (task->vmm != NULL) {
        vmm_release(task->vmm);
    }

    free_cache(task_cache, task);
}

// TODO(frolv): This is very basic for now. There are many more factors to take
// into account.
int task_comparator(const struct task *a, const struct task *b)
{
    if (a->prio_level != b->prio_level) {
        return b->prio_level - a->prio_level;
    }

    // Prefer the task which has less time remaining.
    return a->remaining_time - b->remaining_time;
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

struct task *task_create(const char *path)
{
    int status = 0;

    // Obtain a handle to the executable file, if it exists.
    // TODO(frolv): Use a VFS instead of accessing the initrd directly.
    const struct initrd_file *file = initrd_get_file(path);
    if (!file) {
        return ERR_PTR(ENOENT);
    }

    struct task *task = task_alloc();
    if (IS_ERR(task)) {
        return task;
    }

    struct page *kstack = alloc_page(PA_STANDARD);
    if (IS_ERR(kstack)) {
        status = ERR_VAL(kstack);
        goto error_cleanup;
    }

    task->stack_size = PAGE_SIZE;
    task->stack_top = ((uint8_t *)kstack->mem) + PAGE_SIZE;

    task->vmm = vmm_new();
    if (!task->vmm) {
        status = ENOMEM;
        goto error_cleanup;
    }

    struct elf_context elf;
    if ((status = elf_load(task->vmm, file->base, file->size, &elf))) {
        goto error_cleanup;
    }

    // TODO(frolv): Only the path is set in the command line. Support args.
    task->cmdline = kmalloc(2 * sizeof *task->cmdline);
    if (!task->cmdline) {
        status = ENOMEM;
        goto error_cleanup;
    }

    task->cmdline[0] = strdup(path);
    if (!task->cmdline[0]) {
        status = ENOMEM;
        goto error_cleanup;
    }

    task->cmdline[1] = NULL;

    // Allocate and map a physical user stack into the new task's address space.
    addr_t user_stack_base = USER_STACK_TOP - PAGE_SIZE;
    struct vmm_area *area =
        vmm_alloc_addr(task->vmm, user_stack_base, PAGE_SIZE, VMM_WRITE);
    if (IS_ERR(area)) {
        status = ERR_VAL(area);
        goto error_cleanup;
    }

    struct page *ustack = alloc_page(PA_USER);
    if (IS_ERR(ustack)) {
        status = ERR_VAL(ustack);
        goto error_cleanup;
    }

    if ((status = vmm_map_pages(area, area->base, ustack))) {
        // Must manually free the page here as it won't be owned by the VMM.
        free_pages(ustack);
        goto error_cleanup;
    }

    // Perform architecture-specific setup of the task's user stack and
    // registers.
    if ((status = user_task_setup(task, page_to_phys(ustack), elf.entry))) {
        goto error_cleanup;
    }

    return task;

error_cleanup:
    task_free(task);
    return ERR_PTR(status);
}
