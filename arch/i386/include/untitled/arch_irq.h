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

#ifndef UNTITLED_ARCH_I386_UNTITLED_ARCH_IRQ_H
#define UNTITLED_ARCH_I386_UNTITLED_ARCH_IRQ_H

#define __ARCH_SYSCALL_VECTOR 0x30

void interrupt_disable(void);
void interrupt_enable(void);

#define __arch_irq_disable interrupt_disable
#define __arch_irq_enable  interrupt_enable

#endif /* UNTITLED_ARCH_I386_UNTITLED_ARCH_IRQ_H */
