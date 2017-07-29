/*
 * arch/i386/cpu/exceptions.h
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

#ifndef ARCH_I386_EXCEPTIONS_H
#define ARCH_I386_EXCEPTIONS_H

#include <radix/percpu.h>

DECLARE_PER_CPU(int, unhandled_exceptions);

void div_error(void);
void debug(void);
void breakpoint(void);
void overflow(void);
void bound_range(void);
void invalid_opcode(void);
void device_not_available(void);
void double_fault(void);
void coprocessor_segment(void);
void invalid_tss(void);
void segment_not_present(void);
void stack_segment(void);
void general_protection_fault(void);
void page_fault(void);
void x87_floating_point(void);
void alignment_check(void);
void machine_check(void);
void simd_floating_point(void);
void virtualization_exception(void);
void security_exception(void);

#endif /* ARCH_I386_EXCEPTIONS_H */
