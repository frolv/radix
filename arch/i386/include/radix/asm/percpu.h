/*
 * arch/i386/include/radix/asm/percpu.h
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

#ifndef ARCH_I386_RADIX_PERCPU_H
#define ARCH_I386_RADIX_PERCPU_H

#include <radix/asm/assembler.h>

// Heavily inspired by Linux
// include/linux/percpu-defs.h

#define __ARCH_PER_CPU_SECTION ".percpu_data"
#define __ARCH_PER_CPU_SEGMENT "fs"
#define __ARCH_PER_CPU_REG     "%%" __ARCH_PER_CPU_SEGMENT

#define __percpu_arg(num) \
	__ARCH_PER_CPU_REG ":%" #num

#define __arch_this_cpu_offset() \
	__percpu_from_op("mov", volatile, __this_cpu_offset, 4)

#define this_cpu_read_1(var) __percpu_from_op("mov", volatile, var, 1)
#define this_cpu_read_2(var) __percpu_from_op("mov", volatile, var, 2)
#define this_cpu_read_4(var) __percpu_from_op("mov", volatile, var, 4)

#define this_cpu_write_1(var, val) __percpu_to_op("mov", volatile, var, val, 1)
#define this_cpu_write_2(var, val) __percpu_to_op("mov", volatile, var, val, 2)
#define this_cpu_write_4(var, val) __percpu_to_op("mov", volatile, var, val, 4)

#define this_cpu_add_1(var, val) __percpu_to_op("add", volatile, var, val, 1)
#define this_cpu_add_2(var, val) __percpu_to_op("add", volatile, var, val, 2)
#define this_cpu_add_4(var, val) __percpu_to_op("add", volatile, var, val, 4)

#define this_cpu_sub_1(var, val) __percpu_to_op("sub", volatile, var, val, 1)
#define this_cpu_sub_2(var, val) __percpu_to_op("sub", volatile, var, val, 2)
#define this_cpu_sub_4(var, val) __percpu_to_op("sub", volatile, var, val, 4)

#define raw_cpu_read_1(var) __percpu_from_op("mov", , var, 1)
#define raw_cpu_read_2(var) __percpu_from_op("mov", , var, 2)
#define raw_cpu_read_4(var) __percpu_from_op("mov", , var, 4)

#define raw_cpu_write_1(var, val) __percpu_to_op("mov", , var, val, 1)
#define raw_cpu_write_2(var, val) __percpu_to_op("mov", , var, val, 2)
#define raw_cpu_write_4(var, val) __percpu_to_op("mov", , var, val, 4)

#define raw_cpu_add_1(var, val) __percpu_to_op("add", , var, val, 1)
#define raw_cpu_add_2(var, val) __percpu_to_op("add", , var, val, 2)
#define raw_cpu_add_4(var, val) __percpu_to_op("add", , var, val, 4)

#define raw_cpu_sub_1(var, val) __percpu_to_op("sub", , var, val, 1)
#define raw_cpu_sub_2(var, val) __percpu_to_op("sub", , var, val, 2)
#define raw_cpu_sub_4(var, val) __percpu_to_op("sub", , var, val, 4)

#define __percpu_from_op(op, qualifier, var, size)                      \
({                                                                      \
	typeof(var) __pfo_ret;                                          \
	asm qualifier(op __X86_SUFFIX_##size " " __percpu_arg(1) ", %0" \
	             : "=" __X86_REG_##size(__pfo_ret)                  \
	             : "m"(var));                                       \
	__pfo_ret;                                                      \
})

#define __percpu_to_op(op, qualifier, var, val, size)                \
do {                                                                 \
	__X86_UINTTYPE_##size __pto_val = __X86_CAST_TO_##size(val); \
	asm qualifier(op __X86_SUFFIX_##size " %1, " __percpu_arg(0) \
	    : "+m"(var)                                              \
	    : __X86_REG_IMM_##size(__pto_val));                      \
} while (0)

#include <radix/percpu_defs.h>

DECLARE_PER_CPU(unsigned long, __this_cpu_offset);

void arch_percpu_init_early(void);
int arch_percpu_init(int ap);

#endif  // ARCH_I386_RADIX_PERCPU_H
