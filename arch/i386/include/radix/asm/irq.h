/*
 * arch/i386/include/radix/asm/irq.h
 * Copyright (C) 2021 Alexei Frolov
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

#ifndef ARCH_I386_RADIX_IRQ_H
#define ARCH_I386_RADIX_IRQ_H

#ifndef RADIX_IRQ_H
#error only <radix/irq.h> can be included directly
#endif

#include <radix/asm/pic.h>
#include <radix/asm/vectors.h>
#include <radix/compiler.h>
#include <radix/percpu.h>
#include <radix/types.h>

#define VECTOR_TO_IRQ(vec) ((vec)-IRQ_BASE)
#define IRQ_TO_VECTOR(irq) ((irq) + IRQ_BASE)

#define __arch_irq_init      interrupt_init
#define __arch_in_irq        in_interrupt
#define __arch_irq_install   install_interrupt_handler
#define __arch_irq_uninstall uninstall_interrupt_handler

void interrupt_init(void);
int in_interrupt(void);

DECLARE_PER_CPU(int, interrupt_depth);

int __arch_request_irq(struct irq_descriptor *desc);
int __arch_request_fixed_irq(unsigned int irq,
                             void *device,
                             irq_handler_t handler);
void __arch_release_irq(unsigned int irq, void *device);

static __always_inline void __arch_mask_irq(unsigned int irq)
{
    system_pic->mask(irq);
}

static __always_inline void __arch_unmask_irq(unsigned int irq)
{
    system_pic->unmask(irq);
}

#endif  // ARCH_I386_RADIX_IRQ_H
