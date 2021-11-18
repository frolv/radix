/*
 * kernel/kernel.c
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

#include <radix/boot.h>
#include <radix/compiler.h>
#include <radix/cpu.h>
#include <radix/event.h>
#include <radix/irq.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/kthread.h>
#include <radix/mm.h>
#include <radix/multiboot.h>
#include <radix/percpu.h>
#include <radix/sched.h>
#include <radix/smp.h>
#include <radix/tasking.h>
#include <radix/version.h>
#include <radix/vmm.h>

#include "mm/slab.h"

static void kernel_boot_thread(__unused void *p)
{
	klog(KLOG_INFO, "Post-scheduler boot starting");

	event_start();
	smp_init();

	while (1) {
		HALT();
	}
}

// Kernel entry point.
int kmain(struct multiboot_info *mbt)
{
	klog(KLOG_INFO, KERNEL_NAME " " KERNEL_VERSION);

	buddy_init(mbt);
	slab_init();
	vmm_init();

	arch_main_setup();
	irq_init();
	event_init();
	percpu_area_setup();

	tasking_init();
	irq_enable();

	sched_init();

	// Create a task to continue the boot sequence, then hand over to the
	// scheduler.
	struct task *boot = kthread_create(kernel_boot_thread, NULL, 0,
	                                   "kernel_boot_task");
	boot->cpu_restrict = CPUMASK_SELF;
	kthread_start(boot);

	sched_yield();
	return 0;
}
