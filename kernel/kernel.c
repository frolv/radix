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
#include <radix/initrd.h>
#include <radix/irq.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/kthread.h>
#include <radix/mm.h>
#include <radix/multiboot.h>
#include <radix/percpu.h>
#include <radix/sched.h>
#include <radix/smp.h>
#include <radix/task.h>
#include <radix/version.h>
#include <radix/vmm.h>

#include "mm/slab.h"

static void parse_multiboot_modules(struct multiboot_info *mbt)
{
    multiboot_module_t *modules = (void *)phys_to_virt(mbt->mods_addr);

    for (unsigned i = 0; i < mbt->mods_count; ++i) {
        const char *cmdline = (char *)phys_to_virt(modules[i].cmdline);

        if (strcmp(cmdline, "initrd") == 0) {
            const void *data = (void *)phys_to_virt(modules[i].mod_start);
            size_t size = modules[i].mod_end - modules[i].mod_start;
            read_initrd(data, size);
        } else {
            klog(KLOG_ERROR, "MBT: unknown multiboot module: %s", cmdline);
        }
    }
}

static void kernel_boot_thread(void *p)
{
    struct multiboot_info *mbt = p;

    klog(KLOG_INFO, "%s started", current_task()->cmdline[0]);

    parse_multiboot_modules(mbt);

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
    struct task *boot =
        kthread_create(kernel_boot_thread, mbt, 0, "kernel_boot_task");
    boot->cpu_restrict = CPUMASK_SELF;
    kthread_start(boot);

    sched_yield();
    return 0;
}
