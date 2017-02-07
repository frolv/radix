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

#include <radix/irq.h>
#include <radix/kernel.h>
#include <radix/sched.h>
#include <radix/sys.h>

#include "idt.h"
#include "isr.h"
#include "pic.h"

#define NUM_EXCEPTIONS  32 /* CPU protected mode exceptions */
#define NUM_IRQS        16 /* Industry Standard Architecture IRQs */

/* +1 for syscall interrupt */
#define NUM_INTERRUPTS (NUM_EXCEPTIONS + NUM_IRQS + 1)

#define IRQ_BASE        0x20

/*
 * Routines to be called on interrupt.
 * They fill up a struct regs and then call the interrupt handler proper.
 */
extern void (*intr[NUM_INTERRUPTS])(void);

extern void isr_table_setup(void);

/* interrupt handler functions */
static void (*isr_handlers[256])(struct regs *, unsigned int);

/* hardware interrupt handler functions */
static void (*irq_handlers[NUM_IRQS])(struct regs *);

static void irq_generic(struct regs *r, unsigned int);

void load_interrupt_routines(void)
{
	size_t i;

	isr_table_setup();

	/* add exception routines */
	for (i = 0; i < NUM_EXCEPTIONS; ++i)
		idt_set(i, (uintptr_t)intr[i], 0x08, 0x8E);

	/* add irq routines */
	for (; i < NUM_INTERRUPTS - 1; ++i) {
		idt_set(i, (uintptr_t)intr[i], 0x08, 0x8E);
		isr_handlers[i] = irq_generic;
	}

	/* syscall interrupt */
	idt_set(SYSCALL_INTERRUPT, (uintptr_t)intr[SYSCALL_VECTOR], 0x08, 0x8E);

	/* remap IRQs to vectors 0x20 through 0x2F */
	pic_remap(IRQ_BASE, IRQ_BASE + 8);
}

/* install_interrupt_handler: set a function to handler IRQ `irqno` */
void install_interrupt_handler(uint32_t irqno, void (*hnd)(struct regs *))
{
	if (irqno >= NUM_IRQS)
		return;
	irq_handlers[irqno] = hnd;
}

/* uninstall_interrupt_handler: remove the handler function for `irqno` */
void uninstall_interrupt_handler(uint32_t irqno)
{
	if (irqno >= NUM_IRQS)
		return;

	irq_handlers[irqno] = NULL;
}

/* interrupt_disable: disable IRQs */
void interrupt_disable(void)
{
	asm volatile("cli");

	if (likely(current_task))
		current_task->interrupt_depth++;
}

/* interrupt_enable: enable IRQs */
void interrupt_enable(void)
{
	if (likely(current_task)) {
		if (current_task->interrupt_depth)
			current_task->interrupt_depth--;

		if (!current_task->interrupt_depth)
			asm volatile("sti");
	} else {
		asm volatile("sti");
	}
}

static const char *exceptions[] = {
	"Division by zero",                     /* 0x00 */
	"Debug",                                /* 0x01 */
	"Non-maskable interrupt",               /* 0x02 */
	"Breakpoint",                           /* 0x03 */
	"Overflow",                             /* 0x04 */
	"Out of bounds",                        /* 0x05 */
	"Invalid opcode",                       /* 0x06 */
	"Device not available",                 /* 0x07 */
	"Double fault",                         /* 0x08 */
	"Coprocessor segment overrun",          /* 0x09 */
	"Invalid TSS",                          /* 0x0A */
	"Segment not present",                  /* 0x0B */
	"Stack fault",                          /* 0x0C */
	"General protection fault",             /* 0x0D */
	"Page fault",                           /* 0x0E */
	"Unknown exception",                    /* 0x0F */
	"x87 floating-point exception",         /* 0x10 */
	"Alignment check",                      /* 0x11 */
	"Machine check",                        /* 0x12 */
	"SIMD floating-point exception",        /* 0x13 */
	"Virtualization exception",             /* 0x14 */
	"Unknown exception",                    /* 0x15 */
	"Unknown exception",                    /* 0x16 */
	"Unknown exception",                    /* 0x17 */
	"Unknown exception",                    /* 0x18 */
	"Unknown exception",                    /* 0x19 */
	"Unknown exception",                    /* 0x1A */
	"Unknown exception",                    /* 0x1B */
	"Unknown exception",                    /* 0x1C */
	"Unknown exception",                    /* 0x1D */
	"Security exception",                   /* 0x1E */
	"Unknown exception"                     /* 0x1F */
};

/* TODO: with multiprocessing, this needs to exist per CPU */
static int interrupt = 0;

/*
 * interrupt_handler:
 * Common interrupt handler. Saves registers and calls handler
 * for specific interrupt.
 */
void interrupt_handler(struct interrupt_regs ir)
{
	struct regs r;

	interrupt = 1;
	save_registers(&ir, &r);

	if (isr_handlers[ir.intno]) {
		if (ir.intno > IRQ_BASE)
			isr_handlers[ir.intno](&r, ir.intno);
		else
			isr_handlers[ir.intno](&r, ir.errno);
	} else if (ir.intno < 0x20) {
		panic("unhandled CPU exception 0x%02X `%s'\n",
		      ir.intno, exceptions[ir.intno]);
	}

	load_registers(&ir, &r);
	interrupt = 0;
}

int in_interrupt(void)
{
	return interrupt;
}

static void irq_generic(struct regs *r, unsigned int intno)
{
	if (irq_handlers[intno])
		irq_handlers[intno](r);

	pic_eoi(intno - 0x20);
}
