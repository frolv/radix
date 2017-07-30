/*
 * arch/i386/cpu/isr.c
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

#include <radix/asm/pic.h>
#include <radix/asm/regs.h>

#include <radix/cpu.h>
#include <radix/error.h>
#include <radix/irq.h>
#include <radix/kernel.h>
#include <radix/task.h>

#include "idt.h"

struct pic *system_pic = NULL;

/* hardware interrupt handler functions */
static void (*irq_handlers[NUM_INTERRUPT_VECTORS])(struct regs *) = { NULL };

/* install_interrupt_handler: set a function to handle IRQ `intno` */
int install_interrupt_handler(uint32_t intno, void (*hnd)(struct regs *))
{
	if (intno < IRQ_BASE || intno >= NUM_INTERRUPT_VECTORS)
		return EINVAL;

	irq_handlers[intno] = hnd;
	return 0;
}

/* uninstall_interrupt_handler: remove the handler function for `intno` */
int uninstall_interrupt_handler(uint32_t intno)
{
	if (intno < IRQ_BASE || intno >= NUM_INTERRUPT_VECTORS)
		return EINVAL;

	irq_handlers[intno] = NULL;
	return 0;
}

DEFINE_PER_CPU(int, interrupt_depth) = 0;

/*
 * interrupt_handler:
 * Common interrupt handler. Calls handler function for specified interrupt.
 */
void interrupt_handler(struct regs *regs, int intno)
{
	this_cpu_inc(interrupt_depth);

	system_pic->eoi(intno);
	if (irq_handlers[intno])
		irq_handlers[intno](regs);

	this_cpu_dec(interrupt_depth);
}

int in_interrupt(void)
{
	return !!this_cpu_read(interrupt_depth);
}
