/*
 * include/radix/ipi.h
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

#ifndef RADIX_IPI_H
#define RADIX_IPI_H

#include <radix/asm/ipi.h>
#include <radix/cpumask.h>

#define send_panic_ipi       __arch_send_panic_ipi
#define send_timer_ipi       __arch_send_timer_ipi
#define send_sched_wake(cpu) __arch_send_sched_wake(cpu)

void ipi_init(void);
void arch_ipi_init(void);

#endif /* RADIX_IPI_H */
