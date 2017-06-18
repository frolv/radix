/*
 * arch/i386/include/radix/asm/percpu.h
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

#ifndef ARCH_I386_RADIX_PERCPU_H
#define ARCH_I386_RADIX_PERCPU_H

#define __ARCH_PER_CPU_SECTION ".percpu_data"
#define __ARCH_PER_CPU_SEGMENT "fs"
#define __ARCH_PER_CPU_REG     "%%" __ARCH_PER_CPU_SEGMENT

#define __percpu_arg(num) \
	__ARCH_PER_CPU_REG ":%" #num

#define __arch_this_cpu_offset() \
	__percpu_from_op("movl", __this_cpu_offset)

#define this_cpu_read_1(var)            __percpu_from_op("movb", var)
#define this_cpu_read_2(var)            __percpu_from_op("movw", var)
#define this_cpu_read_4(var)            __percpu_from_op("movl", var)

#define this_cpu_write_1(var, val)      __percpu_to_op("movb", var, val)
#define this_cpu_write_2(var, val)      __percpu_to_op("movw", var, val)
#define this_cpu_write_4(var, val)      __percpu_to_op("movl", var, val)

#define __percpu_from_op(op, var)               \
({                                              \
	typeof(var) __pfo_ret;                  \
	asm(op " " __percpu_arg(1) ", %0"       \
	    : "=q"(__pfo_ret)                   \
	    : "m"(var));                        \
	__pfo_ret;                              \
})

#define __percpu_to_op(op, var, val)            \
do {                                            \
	asm(op " %1, " __percpu_arg(0)          \
	    : "+m"(var)                         \
	    : "ri"(val));                       \
} while (0)

#include <radix/percpu_defs.h>

DECLARE_PER_CPU(unsigned long, __this_cpu_offset);

#endif /* ARCH_I386_RADIX_PERCPU_H */
