/*
 * arch/i386/include/radix/asm/irq.h
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

#ifndef ARCH_I386_RADIX_IRQ_H
#define ARCH_I386_RADIX_IRQ_H

#ifndef RADIX_IRQ_H
#error only <radix/irq.h> can be included directly
#endif

#define ISA_IRQ_COUNT           16
#define IRQ_BASE                0x20

#define VECTOR_TO_IRQ(vec)      ((vec) - IRQ_BASE)
#define IRQ_TO_VECTOR(irq)      ((irq) + IRQ_BASE)

#define __ARCH_TIMER_IRQ        0x0
#define __ARCH_KBD_IRQ          0x1

#define __ARCH_TIMER_VECTOR     IRQ_TO_VECTOR(__ARCH_TIMER_IRQ)
#define __ARCH_KBD_VECTOR       IRQ_TO_VECTOR(__ARCH_KBD_IRQ)
#define __ARCH_SYSCALL_VECTOR   0x80

#define __arch_irq_init         interrupt_init
#define __arch_in_irq           in_interrupt
#define __arch_irq_install      install_interrupt_handler
#define __arch_irq_uninstall    uninstall_interrupt_handler

#define NUM_INTERRUPT_VECTORS   256
#define NUM_EXCEPTION_VECTORS   32

#define X86_EXCEPTION_DE        0x00
#define X86_EXCEPTION_DB        0x01
#define X86_EXCEPTION_BP        0x03
#define X86_EXCEPTION_OF        0x04
#define X86_EXCEPTION_BR        0x05
#define X86_EXCEPTION_UD        0x06
#define X86_EXCEPTION_NM        0x07
#define X86_EXCEPTION_DF        0x08
#define X86_EXCEPTION_CP        0x09
#define X86_EXCEPTION_TS        0x0A
#define X86_EXCEPTION_NP        0x0B
#define X86_EXCEPTION_SS        0x0C
#define X86_EXCEPTION_GP        0x0D
#define X86_EXCEPTION_PF        0x0E
#define X86_EXCEPTION_MF        0x10
#define X86_EXCEPTION_AC        0x11
#define X86_EXCEPTION_MC        0x12
#define X86_EXCEPTION_XM        0x13
#define X86_EXCEPTION_VE        0x14
#define X86_EXCEPTION_SX        0x1E

#ifdef __KERNEL__

#include <radix/asm/pic.h>
#include <radix/compiler.h>
#include <radix/percpu.h>
#include <radix/types.h>

void interrupt_init(void);
int in_interrupt(void);

DECLARE_PER_CPU(int, interrupt_depth);

int __arch_request_irq(struct irq_descriptor *desc);
int __arch_request_fixed_irq(unsigned int irq, void *device,
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

#endif /* __KERNEL__ */

#endif /* ARCH_I386_RADIX_IRQ_H */
