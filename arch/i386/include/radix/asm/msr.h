/*
 * arch/i386/include/radix/asm/msr.h
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

#ifndef ARCH_I386_RADIX_ASM_MSR_H
#define ARCH_I386_RADIX_ASM_MSR_H

#define IA32_TIME_STAMP_COUNTER 0x10
#define IA32_PLATFORM_ID        0x17
#define IA32_APIC_BASE          0x1B
#define IA32_BIOS_UPDT_TRIG     0x79
#define IA32_BIOS_SIGN_ID       0x8B
#define IA32_MTRRCAP            0xFE
#define IA32_X2APIC_APICID      0x802

#include <radix/compiler.h>
#include <radix/types.h>

static __always_inline void rdmsr(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
	asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

static __always_inline void wrmsr(uint32_t msr, uint32_t lo, uint32_t hi)
{
	asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

#endif /* ARCH_I386_RADIX_ASM_MSR_H */
