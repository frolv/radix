/*
 * include/radix/irqstate.h
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

#ifndef RADIX_IRQSTATE_H
#define RADIX_IRQSTATE_H

#include <radix/asm/irqstate.h>

#define irq_state_save()                __arch_irq_state_save()
#define irq_state_restore(state)        __arch_irq_state_restore(state)

#endif /* RADIX_IRQSTATE_H */
