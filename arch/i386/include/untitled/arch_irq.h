/*
 * arch/i386/include/untitled/arch_irq.h
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

#ifndef ARCH_I386_UNTITLED_IRQ_H
#define ARCH_I386_UNTITLED_IRQ_H

#include <untitled/compiler.h>

#define __ARCH_SYSCALL_VECTOR	0x30

#define __arch_irq_active	interrupts_active
#define __arch_irq_disable	interrupt_disable
#define __arch_irq_enable	interrupt_enable
#define __arch_irq_install	install_interrupt_handler
#define __arch_irq_uninstall	uninstall_interrupt_handler

#define __INTERRUPT_BIT (1 << 9)

#include <untitled/sys.h>

void interrupt_disable(void);
void interrupt_enable(void);
void install_interrupt_handler(uint32_t intno, void (*hnd)(struct regs *));
void uninstall_interrupt_handler(uint32_t intno);

#include <stdint.h>

static __always_inline int interrupts_active(void)
{
	uint32_t flags;

	asm volatile("pushf;"
		     "pop %0;"
		     :"=g"(flags));
	return flags & __INTERRUPT_BIT;
}

#endif /* ARCH_I386_UNTITLED_IRQ_H */
