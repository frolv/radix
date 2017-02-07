/*
 * kernel/kthread.c
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

#include <radix/irq.h>
#include <radix/kernel.h>
#include <radix/kthread.h>
#include <radix/mm.h>
#include <radix/slab.h>
#include <radix/task.h>

#include <rlibc/stdio.h>

static struct task *__kthread_create(void (*func)(void *), void *arg,
                                     int page_order);
static void kthread_set_name(struct task *thread, char *name, va_list ap);

/*
 * kthread_create:
 * Create a kernel thread to run function `func` with argument `arg`,
 * and a kernel stack size specified by `page_order`.
 */
struct task *kthread_create(void (*func)(void *), void *arg,
                            int page_order, char *name, ...)
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

	return thread;
}

/* kthread_run: create a kernel thread and immediately start running it */
struct task *kthread_run(void (*func)(void *), void *arg,
                         int page_order, char *name, ...)
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

void kthread_start(struct task *thread)
{
	sched_add(thread);
}

/*
 * kthread_exit: clean up resources and destroy the current thread.
 * This function is called from within a thread to request termination.
 * All created threads set this function as their base return address.
 */
__noreturn void kthread_exit(void)
{
	struct task *thread;
	char **s;

	irq_disable();
	thread = current_task;
	free_pages(thread->stack_base);

	for (s = thread->cmdline; *s; ++s)
		kfree(s);
	kfree(thread->cmdline);

	task_free(thread);
	current_task = NULL;
	schedule(1);
	__builtin_unreachable();
}

static struct task *__kthread_create(void (*func)(void *), void *arg,
                                     int page_order)
{
	struct task *thread;
	struct page *p;
	addr_t stack_top;

	thread = kthread_task();
	if (IS_ERR(thread))
		return thread;

	p = alloc_pages(PA_STANDARD, page_order);
	if (IS_ERR(p)) {
		task_free(thread);
		return (void *)p;
	}

	stack_top = (addr_t)p->mem + POW2(page_order) * PAGE_SIZE - 0x10;
	kthread_reg_setup(&thread->regs, stack_top, (addr_t)func, (addr_t)arg);
	thread->stack_base = p->mem;

	return thread;
}

static void kthread_set_name(struct task *thread, char *name, va_list ap)
{
	thread->cmdline = kmalloc(2 * sizeof *thread->cmdline);
	thread->cmdline[0] = kmalloc(KTHREAD_NAME_LEN);
	vsnprintf(thread->cmdline[0], KTHREAD_NAME_LEN, name, ap);
	thread->cmdline[1] = NULL;
}
