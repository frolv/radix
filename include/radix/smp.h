/*
 * include/radix/smp.h
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

#ifndef RADIX_SMP_H
#define RADIX_SMP_H

#include <radix/cpumask.h>
#include <radix/percpu.h>

DECLARE_PER_CPU(int, processor_id);

#ifdef CONFIG_SMP
#define processor_id() (this_cpu_read(processor_id))
#else
#define processor_id() 0
#endif

#define CPUMASK_CPU(cpu)                ((cpumask_t)(1 << (cpu)))

#define CPUMASK_ALL                     (~((cpumask_t)0))
#define CPUMASK_ALL_BUT(cpu)            (CPUMASK_ALL & ~CPUMASK_CPU(cpu))
#define CPUMASK_ALL_BUT_MASK(cpumask)   (CPUMASK_ALL & ~(cpumask))
#define CPUMASK_ALL_OTHER               CPUMASK_ALL_BUT(processor_id())
#define CPUMASK_SELF                    CPUMASK_CPU(processor_id())

#endif /* RADIX_SMP_H */
