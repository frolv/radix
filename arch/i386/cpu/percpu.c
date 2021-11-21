/*
 * arch/i386/cpu/percpu.c
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

#include <radix/asm/gdt.h>
#include <radix/percpu.h>
#include <radix/smp.h>

#include <stdbool.h>

#include "exceptions.h"

#define BOOT_PERCPU_OFFSET 0

DEFINE_PER_CPU(unsigned long, __this_cpu_offset);

void arch_percpu_init_early(void)
{
    gdt_set_initial_fsbase(BOOT_PERCPU_OFFSET);
    this_cpu_write(__this_cpu_offset, BOOT_PERCPU_OFFSET);
}

// Initialize all architecture-specific per-CPU variables.
int arch_percpu_init(bool is_ap)
{
    addr_t offset;

    if (is_ap) {
        this_cpu_write(unhandled_exceptions, 0);
    } else {
        // Complete BSP per-CPU initialization by setting its
        // fsbase to its newly allocated per-CPU section offset.
        offset = __percpu_offset[processor_id()];
        gdt_set_fsbase(offset);
        this_cpu_write(__this_cpu_offset, offset);
        gdt_init(offset);
    }

    return 0;
}
