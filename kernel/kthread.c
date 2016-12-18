/*
 * kernel/kthread.c
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

#include <stdio.h>
#include <untitled/kernel.h>
#include <untitled/kthread.h>
#include <untitled/mm.h>
#include <untitled/sched.h>

/*
 * kthread_exit: clean up resources and destroy the current thread.
 * This function is called from within a thread to request termination.
 * All created threads set this function as their base return address.
 */
void kthread_exit(void)
{
	struct task *thread;

	thread = current_task;
	free_pages(thread->stack_base);

	/*
	 * TODO: remove thread from scheduler queues
	 * (once scheduler is written), free task struct
	 */
}
