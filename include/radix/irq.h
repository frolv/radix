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

#define SYSCALL_INTERRUPT 0x80

#include <radix/arch_irq.h>

#define SYSCALL_VECTOR  __ARCH_SYSCALL_VECTOR

#define TIMER_IRQ       __ARCH_TIMER_IRQ
#define KBD_IRQ         __ARCH_KBD_IRQ

#define in_irq          __arch_in_irq
#define irq_active      __arch_irq_active
#define irq_disable     __arch_irq_disable
#define irq_enable      __arch_irq_enable
#define irq_install     __arch_irq_install
#define irq_uninstall   __arch_irq_uninstall

#endif /* RADIX_IRQ_H */
