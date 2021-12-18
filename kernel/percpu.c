/*
 * kernel/percpu.c
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

#include <radix/bits.h>
#include <radix/bootmsg.h>
#include <radix/cpu.h>
#include <radix/event.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/mm.h>
#include <radix/percpu.h>
#include <radix/timer.h>
#include <radix/vmm.h>

#include <rlibc/string.h>

extern int __percpu_start;
extern int __percpu_end;

static addr_t percpu_start = (addr_t)&__percpu_start;
static addr_t percpu_end = (addr_t)&__percpu_end;

addr_t __percpu_offset[MAX_CPUS];

void percpu_init_early(void)
{
    memset(__percpu_offset, 0, sizeof __percpu_offset);
    arch_percpu_init_early();
}

/*
 * percpu_init:
 * Initialize all necessary per-CPU variables for each CPU.
 */
int percpu_init(int ap)
{
    int err;

    if ((err = arch_percpu_init(ap)))
        return err;

    if ((err = cpu_timer_init()))
        return err;

    cpu_event_init();

    return 0;
}

/*
 * percpu_area_setup:
 * Allocate memory for per-CPU areas for all CPUs and copy
 * the contents of the per-CPU section into each.
 */
void percpu_area_setup(void)
{
    size_t percpu_size, align, i;
    addr_t percpu_base, base_offset;
    struct vmm_area *area;

    percpu_size = percpu_end - percpu_start;
    if (percpu_size < PAGE_SIZE / 2) {
        align = pow2(log2(percpu_size));
        percpu_size = ALIGN(percpu_size, align);
    } else {
        percpu_size = ALIGN(percpu_size, PAGE_SIZE);
    }

    area = vmm_alloc_size(
        NULL, percpu_size * MAX_CPUS, VMM_READ | VMM_WRITE | VMM_ALLOC_UPFRONT);
    if (IS_ERR(area)) {
        panic("failed to allocate space for per-CPU areas\n");
    }

    percpu_base = area->base;
    base_offset = percpu_base - percpu_start;

    for (i = 0; i < MAX_CPUS; ++i) {
        __percpu_offset[i] = base_offset + i * percpu_size;
        memcpy((void *)percpu_base, (void *)percpu_start, percpu_size);
        percpu_base += percpu_size;
    }

    /* initialize per-CPU variables for the BSP */
    percpu_init(0);

    /*
     * TODO: we no longer need the original per-CPU area, so we can add
     * it to the page allocator. (Requires adding an additional zone_init
     * call in buddy_populate of page.c to handle the .percpu_data section.)
     */

    if (percpu_size < PAGE_SIZE) {
        klog(KLOG_INFO,
             "percpu: allocated %uB for %d CPUs (%uB per CPU)\n",
             percpu_size * MAX_CPUS,
             MAX_CPUS,
             percpu_size);
    } else {
        klog(KLOG_INFO,
             "percpu: allocated %u pages for %d CPUs "
             "(%u page%s per CPU)\n",
             percpu_size / PAGE_SIZE * MAX_CPUS,
             MAX_CPUS,
             percpu_size / PAGE_SIZE,
             percpu_size > PAGE_SIZE ? "s" : "");
    }
}
