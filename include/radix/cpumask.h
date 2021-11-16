/*
 * include/radix/cpumask.h
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

#ifndef RADIX_CPUMASK_H
#define RADIX_CPUMASK_H

#include <radix/config.h>
#include <radix/types.h>

#define MAX_CPUS CONFIG(MAX_CPUS)

#if MAX_CPUS > 32
typedef uint64_t cpumask_t;
#else
typedef uint32_t cpumask_t;
#endif

#endif  // RADIX_CPUMASK_H
