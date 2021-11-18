/*
 * arch/i386/include/radix/asm/assembler.h
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

#ifndef ARCH_I386_RADIX_ASSEMBLER_H
#define ARCH_I386_RADIX_ASSEMBLER_H

#ifdef __ASSEMBLY__

#define __PERCPU_SECTION .percpu_data
#define __PERCPU_SEGMENT %fs

#define DEFINE_PER_CPU(var)                     \
	.section __PERCPU_SECTION ASM_NL        \
	.global var ASM_NL                      \
	var:

#define DEFINE_PER_CPU_END()                    \
	.section .text ASM_NL                   \
	.align 4

#define THIS_CPU_VAR(var) __PERCPU_SEGMENT:var

#endif  // __ASSEMBLY__

#ifdef __KERNEL__

// Helpers for writing inline assembly.

#define __X86_LOCK "lock; "

#define __X86_UINTTYPE_1 uint8_t
#define __X86_UINTTYPE_2 uint16_t
#define __X86_UINTTYPE_4 uint32_t

#define __X86_SUFFIX_1 "b"
#define __X86_SUFFIX_2 "w"
#define __X86_SUFFIX_4 "l"

#define __X86_CAST_TO_1(val) ((__X86_UINTTYPE_1)(((unsigned long)val) & 0xff))
#define __X86_CAST_TO_2(val) ((__X86_UINTTYPE_2)(((unsigned long)val) & 0xffff))
#define __X86_CAST_TO_4(val) \
	((__X86_UINTTYPE_4)(((unsigned long)val) & 0xffffffff))

#define __X86_REG_1(val) "q"(val)
#define __X86_REG_2(val) "r"(val)
#define __X86_REG_4(val) "r"(val)

#define __X86_REG_IMM_1(val) "qi"(val)
#define __X86_REG_IMM_2(val) "ri"(val)
#define __X86_REG_IMM_4(val) "ri"(val)

#endif  // __KERNEL__

#endif  // ARCH_I386_RADIX_ASSEMBLER_H
