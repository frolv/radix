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

#include "idt.h"
#include "isr.h"
#include "pic8259.h"

static uint64_t idt[IDT_ENTRIES];

static void (*early_isr_fn[])(void) = {
	early_isr_0, early_isr_1, early_isr_2, early_isr_3,
	early_isr_4, early_isr_5, early_isr_6, early_isr_7,
	early_isr_8, early_isr_9, early_isr_10, early_isr_11,
	early_isr_12, early_isr_13, early_isr_14, early_isr_15,
	early_isr_16, early_isr_17, early_isr_18, early_isr_19,
	early_isr_20, early_isr_21, early_isr_22, early_isr_23,
	early_isr_24, early_isr_25, early_isr_26, early_isr_27,
	early_isr_28, early_isr_29, early_isr_30, early_isr_31
};

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
void idt_set(size_t intno, uintptr_t intfn, uint16_t sel, uint8_t flags)
{
	idt[intno] = idt_pack(intfn, sel, flags);
}

/*
 * idt_init_early:
 * Load an interrupt descriptor table containing interrupt handler
 * stubs for use during early boot sequence.
 */
void idt_init_early(void)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(early_isr_fn); ++i)
		idt[i] = idt_pack((uintptr_t)early_isr_fn[i], 0x08, 0x8E);

	pic8259_remap(IRQ_BASE, IRQ_BASE + 8);
	pic8259_disable();

	idt_load(idt, sizeof idt);
}
