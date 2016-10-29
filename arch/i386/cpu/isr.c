/*
 * arch/i386/cpu/isr.c
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

#include <untitled/sys.h>
#include "idt.h"
#include "isr.h"

#define NUM_EXCEPTIONS	32 /* CPU protected mode exceptions */
#define NUM_IRQS	16 /* Industry Standard Architecture IRQs */

/* +1 for syscall interrupt */
#define NUM_INTERRUPTS (NUM_EXCEPTIONS + NUM_IRQS + 1)

/*
 * Routines to be called on interrupt.
 * They fill up a struct regs and then call the interrupt handler proper.
 */
extern void (*intr[NUM_INTERRUPTS])(void);

extern void isr_table_setup(void);

/* interrupt handler functions */
static void (*isr_handlers[256])(struct regs *regs);

void load_interrupt_routines(void)
{
	size_t i;

	isr_table_setup();

	for (i = 0; i < NUM_INTERRUPTS - 1; ++i)
		idt_set(i, (uintptr_t)intr[i], 0x08, 0x8E);

	/* syscall interrupt */
	idt_set(0x80, (uintptr_t)intr[i], 0x08, 0x8E);
}

void interrupt_handler(struct regs r)
{
	if (isr_handlers[r.intno]) {
		/* isr_handlers[r.intno](&r); */
	} else {
		/* printf("Error: unhandled interrupt %d\n", r.intno); */
		/* panic(); */
	}
}
