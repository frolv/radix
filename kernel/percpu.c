/*
 * kernel/percpu.c
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

#include <radix/bits.h>
#include <radix/cpu.h>
#include <radix/kernel.h>
#include <radix/mm.h>
#include <radix/percpu.h>
#include <radix/slab.h>

#include <rlibc/string.h>

extern int __percpu_start;
extern int __percpu_end;

static addr_t percpu_start = (addr_t)&__percpu_start;
static addr_t percpu_end = (addr_t)&__percpu_end;

addr_t __percpu_offset[MAX_CPUS];

void percpu_init_early(void)
{
	memset(__percpu_offset, 0, sizeof __percpu_offset);
	arch_percpu_init_early();
}
