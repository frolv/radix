/*
 * arch/i386/irq/interrupts.c
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

#include <radix/asm/apic.h>
#include <radix/asm/regs.h>
#include <radix/cpu.h>
#include <radix/error.h>
#include <radix/ipi.h>
#include <radix/irq.h>
#include <radix/kernel.h>
#include <radix/slab.h>
#include <radix/task.h>

#include <rlibc/string.h>

struct pic *system_pic = NULL;

static void irq_nop(void *device);

#define IRQ_RESERVED (1 << 16)

/* if no other vectors can be shared, this vector is used */
#define DEFAULT_SHARED_VECTOR IRQ_TO_VECTOR(9)

/* interrupt handler functions */
static struct irq_descriptor irq_descriptors[NUM_INTERRUPT_VECTORS] = {
    [0 ... NUM_INTERRUPT_VECTORS - 1] = {
        .handler = irq_nop, .device = NULL, .flags = 0, .next = NULL}};

static uint8_t num_irq_descriptors[NUM_INTERRUPT_VECTORS] = {
    [0 ... NUM_INTERRUPT_VECTORS - 1] = 1};

static unsigned int next_shared_vector;

static void __add_irq_desc(struct irq_descriptor *head,
                           struct irq_descriptor *desc)
{
    for (; head->next; head = head->next)
        ;
    head->next = desc;
}

static void __del_irq_desc(struct irq_descriptor **head,
                           struct irq_descriptor *desc)
{
    for (; *head && *head != desc; head = &(*head)->next)
        ;
    if (!*head)
        return;

    *head = desc->next;
}

static struct irq_descriptor *__find_irq_desc(int vector, void *device)
{
    struct irq_descriptor *desc = &irq_descriptors[vector];

    for (; desc && desc->device != device; desc = desc->next)
        ;
    return desc;
}

/*
 * __find_available_vector:
 * Try to find an available interrupt vector for a device.
 */
static int __find_available_vector(unsigned long flags)
{
    struct irq_descriptor *desc;
    unsigned int vector;

    vector = IRQ_BASE + system_pic->irq_count;
    for (; vector < NUM_INTERRUPT_VECTORS; ++vector) {
        desc = &irq_descriptors[vector];
        if (!(desc->flags & IRQ_RESERVED) && desc->handler == irq_nop)
            return vector;
    }

    /* no IRQs available and device doesn't allow sharing */
    if (!(flags & IRQ_ALLOW_SHARED))
        return -EBUSY;

    if (!next_shared_vector)
        return DEFAULT_SHARED_VECTOR;

    return next_shared_vector++;
}

static void __update_next_shared_vector(void)
{
    struct irq_descriptor *desc;
    unsigned int end;

    if (!next_shared_vector) {
        next_shared_vector = IRQ_BASE + system_pic->irq_count;
        end = NUM_INTERRUPT_VECTORS - 1;
    } else if (next_shared_vector == IRQ_BASE + system_pic->irq_count) {
        end = NUM_INTERRUPT_VECTORS - 1;
    } else {
        end = next_shared_vector - 1;
    }

    for (; next_shared_vector != end; ++next_shared_vector) {
        if (next_shared_vector >= NUM_INTERRUPT_VECTORS)
            next_shared_vector = IRQ_BASE + system_pic->irq_count;

        desc = &irq_descriptors[next_shared_vector];
        if (!(desc->flags & IRQ_RESERVED) &&
            ((desc->flags & IRQ_ALLOW_SHARED) || desc->handler == irq_nop))
            return;
    };

    /* no vectors can be shared; DEFAULT_SHARED_VECTOR will be used */
    next_shared_vector = 0;
}

int __arch_request_irq(struct irq_descriptor *desc)
{
    int vector;

    vector = __find_available_vector(desc->flags);
    if (vector < 0)
        return vector;

    if (irq_descriptors[vector].handler == irq_nop) {
        memcpy(&irq_descriptors[vector], desc, sizeof *desc);
        kfree(desc);
    } else {
        __add_irq_desc(&irq_descriptors[vector], desc);
        ++num_irq_descriptors[vector];
    }
    __update_next_shared_vector();

    return VECTOR_TO_IRQ(vector);
}

int __arch_request_fixed_irq(unsigned int irq,
                             void *device,
                             irq_handler_t handler)
{
    struct irq_descriptor *desc;
    unsigned int vector;

    if (irq >= system_pic->irq_count)
        return EINVAL;

    vector = IRQ_TO_VECTOR(irq);
    if (irq_descriptors[vector].handler == irq_nop) {
        desc = &irq_descriptors[vector];
    } else {
        desc = kmalloc(sizeof *desc);
        if (!desc)
            return ENOMEM;

        __add_irq_desc(&irq_descriptors[vector], desc);
        ++num_irq_descriptors[vector];
    }

    desc->handler = handler;
    desc->device = device;
    desc->next = NULL;

    return 0;
}

/*
 * __arch_release_irq:
 * Remove the IRQ handler for the specified device and IRQ.
 */
void __arch_release_irq(unsigned int irq, void *device)
{
    struct irq_descriptor *desc, *tmp;
    unsigned int vector;

    vector = IRQ_TO_VECTOR(irq);
    desc = __find_irq_desc(vector, device);
    if (!desc)
        return;

    if (desc == &irq_descriptors[vector]) {
        if (desc->next) {
            tmp = desc->next;
            memcpy(desc, desc->next, sizeof *desc);
            --num_irq_descriptors[vector];
            kfree(tmp);
        } else {
            desc->handler = irq_nop;
            desc->device = NULL;
            desc->flags = 0;
            if (irq <= system_pic->irq_count)
                system_pic->mask(irq);
        }
    } else {
        __del_irq_desc(&irq_descriptors[vector].next, desc);
        kfree(desc);
        --num_irq_descriptors[vector];
    }
}

/*
 * interrupt_handler:
 * Common interrupt handler. Calls handler functions for specified interrupt.
 */
void interrupt_handler(__unused struct regs *regs, int intno)
{
    struct irq_descriptor *desc;

    system_pic->eoi(intno);

    desc = &irq_descriptors[intno];
    for (; desc; desc = desc->next)
        desc->handler(desc->device);
}

int in_interrupt(void) { return !!this_cpu_read(interrupt_depth); }

void interrupt_init(void)
{
    size_t irq;

    /* reserve all PIC interrupts */
    for (irq = 0; irq < system_pic->irq_count; ++irq)
        irq_descriptors[IRQ_TO_VECTOR(irq)].flags |= IRQ_RESERVED;

    irq_descriptors[APIC_VEC_NMI].flags |= IRQ_RESERVED;
    irq_descriptors[APIC_VEC_SMI].flags |= IRQ_RESERVED;
    irq_descriptors[APIC_VEC_EXTINT].flags |= IRQ_RESERVED;
    irq_descriptors[APIC_VEC_ERROR].flags |= IRQ_RESERVED;
    irq_descriptors[APIC_VEC_THERMAL].flags |= IRQ_RESERVED;
    irq_descriptors[APIC_VEC_CMCI].flags |= IRQ_RESERVED;
    irq_descriptors[APIC_VEC_SPURIOUS].flags |= IRQ_RESERVED;

    irq_descriptors[IPI_VEC_PANIC].flags |= IRQ_RESERVED;
    irq_descriptors[IPI_VEC_TLB_SHOOTDOWN].flags |= IRQ_RESERVED;
    irq_descriptors[IPI_VEC_TIMER_ACTION].flags |= IRQ_RESERVED;
    irq_descriptors[IPI_VEC_SCHED_WAKE].flags |= IRQ_RESERVED;

    next_shared_vector = IRQ_BASE + system_pic->irq_count;
}

/* irq_nop: no-op IRQ handler */
static void irq_nop(__unused void *device) {}
