/*
 * kernel/cpumask.c
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

#include <radix/smp.h>

static cpumask_t online_cpus = CPUMASK_CPU(0);

cpumask_t cpumask_online(void)
{
	return online_cpus;
}

void set_cpu_online(unsigned int cpu)
{
	online_cpus |= CPUMASK_CPU(cpu);
}
