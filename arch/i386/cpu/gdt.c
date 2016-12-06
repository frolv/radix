/*
 * arch/i386/cpu/gdt.c
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

#include <string.h>
#include <untitled/types.h>

/* Global Descriptor Table */
static uint64_t gdt[6];
/* Task State Segment */
static uint32_t tss[26];

extern void gdt_load(void *base, size_t s);
extern void tss_load(uintptr_t gdt_offset);

static void tss_init(uint32_t esp0, uint32_t ss0);
static void gdt_set(size_t entry, uint32_t base, uint32_t lim,
		    int8_t access, uint8_t flags);

/* gdt_init: populate the global descriptor table */
void gdt_init(void)
{
	uintptr_t tss_base;

	tss_base = (uintptr_t)tss;
	tss_init(0x0, 0x10);

	gdt_set(0, 0, 0, 0, 0);
	gdt_set(1, 0, 0xFFFFFFFF, 0x9A, 0x0C); /* ring level 0 code */
	gdt_set(2, 0, 0xFFFFFFFF, 0x92, 0x0C); /* ring level 0 data */
	gdt_set(3, 0, 0xFFFFFFFF, 0xFA, 0x0C); /* ring level 3 code */
	gdt_set(4, 0, 0xFFFFFFFF, 0xF2, 0x0C); /* ring level 3 data */
	gdt_set(5, tss_base, tss_base + sizeof tss, 0x89, 0x04);

	gdt_load(gdt, sizeof gdt);
	tss_load(0x28);
}

/*
 * tss_init:
 * Initialize the task state segment.
 * ESP0 holds the value assigned to the stack pointer after a syscall interrupt.
 * SS0 holds the offset of the kernel's data segment entry in the GDT.
 */
static void tss_init(uint32_t esp0, uint32_t ss0)
{
	memset(tss, 0, sizeof tss);

	/* ESP0 at offset 0x4 */
	tss[1] = esp0;
	/* SS0 at offset 0x8 */
	tss[2] = ss0;
	/* IOPB at offset 0x66 */
	tss[25] = sizeof tss << 16;
}

/* gdt_set: create an entry in the global descriptor table */
static void gdt_set(size_t entry, uint32_t base, uint32_t lim,
		    int8_t access, uint8_t flags)
{
	/* top half of entry */
	gdt[entry] = lim & 0x000F0000;
	gdt[entry] |= ((uint32_t)flags << 20) & 0x00F00000;
	gdt[entry] |= ((uint16_t)access << 8) & 0xFF00;
	gdt[entry] |= (base >> 16) & 0x000000FF;
	gdt[entry] |= base & 0xFF000000;

	/* bottom half of entry */
	gdt[entry] <<= 32;
	gdt[entry] |= lim & 0x0000FFFF;
	gdt[entry] |= base << 16;
}
