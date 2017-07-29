/*
 * arch/i386/include/radix/asm/assembler.h
 * Copyright (C) 2017 Alexei Frolov
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

#define THIS_CPU_VAR(var) __PERCPU_SEGMENT:var

#endif /* __ASSEMBLY__ */

#endif /* ARCH_I386_RADIX_ASSEMBLER_H */
