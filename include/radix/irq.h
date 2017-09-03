/*
 * include/radix/irq.h
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

#ifndef RADIX_IRQ_H
#define RADIX_IRQ_H

#ifdef __KERNEL__

#include <radix/compiler.h>

typedef void (*irq_handler_t)(void *);

struct irq_descriptor {
	irq_handler_t           handler;
	void                    *device;
	unsigned long           flags;
	struct irq_descriptor   *next;
};

#define IRQ_ALLOW_SHARED        (1 << 0)

__must_check int request_irq(void *device, irq_handler_t handler,
                             unsigned long flags);
__must_check int request_fixed_irq(unsigned int irq, void *device,
                                   irq_handler_t handler);
void release_irq(unsigned int irq, void *device);

#define mask_irq(irq)   __arch_mask_irq(irq)
#define unmask_irq(irq) __arch_unmask_irq(irq)

#endif /* __KERNEL__ */

#include <radix/asm/irq.h>
#include <radix/irqstate.h>

#define SYSCALL_INTERRUPT 0x80

#define SYSCALL_VECTOR  __ARCH_SYSCALL_VECTOR

#define irq_init        __arch_irq_init
#define in_irq          __arch_in_irq

#endif /* RADIX_IRQ_H */
