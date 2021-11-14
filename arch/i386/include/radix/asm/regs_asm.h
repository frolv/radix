/*
 * arch/i386/include/radix/asm/regs_asm.h
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

#ifndef ARCH_I386_RADIX_REGS_ASM_H
#define ARCH_I386_RADIX_REGS_ASM_H

#include <radix/preprocessor.h>

#ifdef __ASSEMBLY__

// Macros which expand to an assembly operand for a specific register within a
// `struct regs` located at some base address.
//
//   mov $42, REGS_AX(%esp)  ->  mov $42, 28(%esp)
//
// An additional offset can be provided to the macro, in case there isn't a
// direct pointer to a struct regs (e.g. it's nested in another struct).
//
//   mov REGS_SP(%edx, 100), %esp  ->  mov 108(%edx), %esp
//
#define REGS_DI(base, ...)   \
	__REGS_OFFSET_COUNT( \
		__OFFSET_DI, base, ARG_COUNT(##__VA_ARGS__), ##__VA_ARGS__)

#define REGS_SI(base, ...)   \
	__REGS_OFFSET_COUNT( \
		__OFFSET_SI, base, ARG_COUNT(##__VA_ARGS__), ##__VA_ARGS__)

#define REGS_SP(base, ...)   \
	__REGS_OFFSET_COUNT( \
		__OFFSET_SP, base, ARG_COUNT(##__VA_ARGS__), ##__VA_ARGS__)

#define REGS_BP(base, ...)   \
	__REGS_OFFSET_COUNT( \
		__OFFSET_BP, base, ARG_COUNT(##__VA_ARGS__), ##__VA_ARGS__)

#define REGS_BX(base, ...)   \
	__REGS_OFFSET_COUNT( \
		__OFFSET_BX, base, ARG_COUNT(##__VA_ARGS__), ##__VA_ARGS__)

#define REGS_DX(base, ...)   \
	__REGS_OFFSET_COUNT( \
		__OFFSET_DX, base, ARG_COUNT(##__VA_ARGS__), ##__VA_ARGS__)

#define REGS_CX(base, ...)   \
	__REGS_OFFSET_COUNT( \
		__OFFSET_CX, base, ARG_COUNT(##__VA_ARGS__), ##__VA_ARGS__)

#define REGS_AX(base, ...)   \
	__REGS_OFFSET_COUNT( \
		__OFFSET_AX, base, ARG_COUNT(##__VA_ARGS__), ##__VA_ARGS__)

#define REGS_GS(base, ...)   \
	__REGS_OFFSET_COUNT( \
		__OFFSET_GS, base, ARG_COUNT(##__VA_ARGS__), ##__VA_ARGS__)

#define REGS_FS(base, ...)   \
	__REGS_OFFSET_COUNT( \
		__OFFSET_FS, base, ARG_COUNT(##__VA_ARGS__), ##__VA_ARGS__)

#define REGS_ES(base, ...)   \
	__REGS_OFFSET_COUNT( \
		__OFFSET_ES, base, ARG_COUNT(##__VA_ARGS__), ##__VA_ARGS__)

#define REGS_DS(base, ...)   \
	__REGS_OFFSET_COUNT( \
		__OFFSET_DS, base, ARG_COUNT(##__VA_ARGS__), ##__VA_ARGS__)

#define REGS_CS(base, ...)   \
	__REGS_OFFSET_COUNT( \
		__OFFSET_CS, base, ARG_COUNT(##__VA_ARGS__), ##__VA_ARGS__)

#define REGS_SS(base, ...)   \
	__REGS_OFFSET_COUNT( \
		__OFFSET_SS, base, ARG_COUNT(##__VA_ARGS__), ##__VA_ARGS__)

#define REGS_IP(base, ...)   \
	__REGS_OFFSET_COUNT( \
		__OFFSET_IP, base, ARG_COUNT(##__VA_ARGS__), ##__VA_ARGS__)

#define REGS_FLAGS(base, ...) \
	__REGS_OFFSET_COUNT(  \
		__OFFSET_FLAGS, base, ARG_COUNT(##__VA_ARGS__), ##__VA_ARGS__)

// The offsets of values within an x86 `struct regs` (asm/regs.h) from a base.
#define __OFFSET_DI     0
#define __OFFSET_SI     4
#define __OFFSET_SP     8
#define __OFFSET_BP    12
#define __OFFSET_BX    16
#define __OFFSET_DX    20
#define __OFFSET_CX    24
#define __OFFSET_AX    28
#define __OFFSET_GS    32
#define __OFFSET_FS    36
#define __OFFSET_ES    40
#define __OFFSET_DS    44
#define __OFFSET_CS    48
#define __OFFSET_SS    52
#define __OFFSET_IP    56
#define __OFFSET_FLAGS 60

#define __REGS_OFFSET(reg_offset, base, addl_offset) \
	(reg_offset + addl_offset)(base)

#define __REGS_OFFSET_0(reg_offset, base, ...) \
	__REGS_OFFSET(reg_offset, base, 0)
#define __REGS_OFFSET_1(reg_offset, base, ...) \
	__REGS_OFFSET(reg_offset, base, __VA_ARGS__)

#define __REGS_OFFSET_COUNT_EVAL(reg_offset, base, count, ...) \
	__REGS_OFFSET_##count(reg_offset, base, __VA_ARGS__)
#define __REGS_OFFSET_COUNT(reg_offset, base, count, ...) \
	__REGS_OFFSET_COUNT_EVAL(reg_offset, base, count, __VA_ARGS__)

#endif // __ASSEMBLY__

#endif // ARCH_I386_RADIX_REGS_ASM_H

