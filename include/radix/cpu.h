/*
 * include/radix/cpu.h
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

#ifndef RADIX_CPU_H
#define RADIX_CPU_H

#include <radix/asm/cpu.h>

#define cpu_cache_line_size()   __arch_cache_line_size()
#define cpu_set_kernel_stack(s) __arch_set_kernel_stack(s)
#define cpu_cache_str()         __arch_cache_str()

#endif /* RADIX_CPU_H */
