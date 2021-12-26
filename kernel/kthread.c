/*
 * kernel/kthread.c
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
#include <radix/bits.h>
#include <radix/irq.h>
#include <radix/kthread.h>
#include <radix/mm.h>
#include <radix/sched.h>
#include <radix/slab.h>
#include <radix/task.h>
#include <radix/vmm.h>

#include <rlibc/stdio.h>

static struct task *__kthread_create(void (*func)(void *),
                                     void *arg,
                                     int page_order);
static void kthread_set_name(struct task *thread, char *name, va_list ap);

// Creates a kernel thread to run function `func` with argument `arg`,
// and a kernel stack with size specified by `page_order`.
//
// The thread is not run automatically; kthread_start must be called first.
struct task *kthread_create(
    void (*func)(void *), void *arg, int page_order, char *name, ...)
{
    struct task *thread;
    va_list ap;

    if (unlikely(!func || !name))
        return ERR_PTR(EINVAL);

    thread = __kthread_create(func, arg, page_order);
    if (IS_ERR(thread))
        return thread;

    va_start(ap, name);
    kthread_set_name(thread, name, ap);
    va_end(ap);

    return thread;
}

// Creates a kernel thread and immediately starts running it.
struct task *kthread_run(
    void (*func)(void *), void *arg, int page_order, char *name, ...)
{
    struct task *thread;
    va_list ap;

    if (unlikely(!name))
        return ERR_PTR(EINVAL);

    thread = __kthread_create(func, arg, page_order);
    if (IS_ERR(thread))
        return thread;

    va_start(ap, name);
    kthread_set_name(thread, name, ap);
    va_end(ap);

    kthread_start(thread);
    return thread;
}

void kthread_start(struct task *thread) { sched_add(thread); }

// Exits the running kthread.
// All created kthreads set this function as their base return address.
__noreturn void kthread_exit(void)
{
    irq_disable();

    struct task *thread = current_task();
    assert(thread);
    task_exit(thread, 0);

    __builtin_unreachable();
}

static struct task *__kthread_create(void (*func)(void *),
                                     void *arg,
                                     int page_order)
{
    struct task *thread;
    struct page *p;
    addr_t stack_top;

    thread = task_alloc();
    if (IS_ERR(thread)) {
        return thread;
    }

    p = alloc_pages(PA_STANDARD, page_order);
    if (IS_ERR(p)) {
        task_free(thread);
        return (void *)p;
    }

    thread->vmm = vmm_kernel();

    thread->stack_size = pow2(page_order) * PAGE_SIZE;
    stack_top = (addr_t)p->mem + thread->stack_size;
    kthread_reg_setup(&thread->regs, stack_top, (addr_t)func, (addr_t)arg);
    thread->stack_top = (void *)stack_top;

    return thread;
}

static void kthread_set_name(struct task *thread, char *name, va_list ap)
{
    thread->cmdline = kmalloc(2 * sizeof *thread->cmdline);
    thread->cmdline[0] = kmalloc(KTHREAD_NAME_LEN);
    vsnprintf(thread->cmdline[0], KTHREAD_NAME_LEN, name, ap);
    thread->cmdline[1] = NULL;
}
