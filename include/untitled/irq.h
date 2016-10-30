/*
 * include/untitled/irq.h
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

#ifndef UNTITLED_UNTITLED_IRQ_H
#define UNTITLED_UNTITLED_IRQ_H

#define SYSCALL_INTERRUPT 0x80

#include <untitled/arch_irq.h>

#define SYSCALL_VECTOR __ARCH_SYSCALL_VECTOR

#define irq_disable __arch_irq_disable
#define irq_enable  __arch_irq_enable

#endif /* UNTITLED_UNTITLED_IRQ_H */
