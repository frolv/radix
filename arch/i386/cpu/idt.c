/*
 * arch/i386/cpu/idt.c
 * Copyright (C) 2016 Alexei Frolov
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

#include "idt.h"
#include "isr.h"

/* Interrupt Descriptor Table */
static uint64_t idt[256];

extern void idt_load(void *base, size_t s);

void idt_init(void)
{
	load_interrupt_routines();
	idt_load(idt, sizeof idt);
}

/* idt_set: load a single entry into the IDT */
void idt_set(size_t intno, uintptr_t intfn, uint16_t sel, uint8_t flags)
{
	idt[intno] = intfn & 0xFFFF0000;
	idt[intno] |= ((uint16_t)flags << 8) & 0xFF00;

	idt[intno] <<= 32;
	idt[intno] |= (uint32_t)sel << 16;
	idt[intno] |= intfn & 0x0000FFFF;
}
