/*
 * kernel/cpumask.c
 * Copyright (C) 2021 Alexei Frolov
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

#include <radix/atomic.h>
#include <radix/smp.h>

static cpumask_t online_cpus = CPUMASK_CPU(0);
static cpumask_t idle_cpus = 0;

cpumask_t cpumask_online(void)
{
	return online_cpus;
}

cpumask_t cpumask_idle(void)
{
	return idle_cpus;
}

void set_cpu_online(int cpu)
{
	atomic_or(&online_cpus, CPUMASK_CPU(cpu));
}

void set_cpu_offline(int cpu)
{
	atomic_and(&online_cpus, ~CPUMASK_CPU(cpu));
}

void set_cpu_idle(int cpu)
{
	atomic_or(&idle_cpus, CPUMASK_CPU(cpu));
}

void set_cpu_active(int cpu)
{
	atomic_and(&idle_cpus, ~CPUMASK_CPU(cpu));
}
