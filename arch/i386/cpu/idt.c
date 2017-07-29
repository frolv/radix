/*
 * arch/i386/cpu/idt.c
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

#include <radix/irq.h>
#include <radix/kernel.h>
#include <radix/percpu.h>

#include "exceptions.h"
#include "idt.h"
#include "isr.h"
#include "pic8259.h"

static uint64_t idt[IDT_ENTRIES];

extern void idt_load(void *base, size_t s);

void idt_init(void)
{
	load_interrupt_routines();
}

static uint64_t idt_pack(uintptr_t intfn, uint16_t sel, uint8_t flags)
{
	uint64_t ret;

	ret = intfn & 0xFFFF0000;
	ret |= ((uint16_t)flags << 8) & 0xFF00;

	ret <<= 32;
	ret |= (uint32_t)sel << 16;
	ret |= intfn & 0x0000FFFF;

	return ret;
}

/* idt_set: load a single entry into the IDT */
void idt_set(size_t intno, void (*intfn)(void), uint16_t sel, uint8_t flags)
{
	idt[intno] = idt_pack((uintptr_t)intfn, sel, flags);
}

/*
 * idt_init_early:
 * Load an interrupt descriptor table containing interrupt handlers
 * for CPU exceptions.
 */
void idt_init_early(void)
{
	idt_set(X86_EXCEPTION_DE, div_error, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_DB, debug, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_BP, breakpoint, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_OF, overflow, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_BR, bound_range, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_UD, invalid_opcode, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_NM, device_not_available, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_DF, double_fault, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_CP, coprocessor_segment, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_TS, invalid_tss, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_NP, segment_not_present, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_SS, stack_segment, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_GP, general_protection_fault, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_PF, page_fault, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_MF, x87_floating_point, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_AC, alignment_check, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_MC, machine_check, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_XM, simd_floating_point, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_VE, virtualization_exception, 0x08, 0x8E);
	idt_set(X86_EXCEPTION_SX, security_exception, 0x08, 0x8E);

	pic8259_init();
	pic8259_disable();

	idt_load(idt, sizeof idt);
}
