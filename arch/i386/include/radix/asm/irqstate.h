/*
 * arch/i386/include/radix/asm/irqstate.h
 * Copyright (C) 2017 Alexei Frolov
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

#ifndef ARCH_I386_RADIX_IRQSTATE_H
#define ARCH_I386_RADIX_IRQSTATE_H

#ifndef RADIX_IRQSTATE_H
#error only <radix/irqstate.h> can be included directly
#endif

#include <radix/asm/cpu_defs.h>

#define __arch_irq_state_save() (cpu_read_flags() & EFLAGS_IF)
#define __arch_irq_state_restore(state) \
	cpu_update_flags(EFLAGS_IF, (state) & EFLAGS_IF)

#endif /* ARCH_I386_RADIX_IRQSTATE_H */
