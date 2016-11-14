/*
 * include/untitled/kernel.h
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

#ifndef UNTITLED_KERNEL_H
#define UNTITLED_KERNEL_H

#include <stddef.h>

#define HALT()  asm volatile("hlt")
#define DIE() \
	do { \
		HALT(); \
	} while (1)

#define ALIGN(x, a)		__ALIGN_MASK(x, (typeof (x))(a) - 1)
#define __ALIGN_MASK(x, mask)	(((x) + (mask)) & ~(mask))

#define ALIGNED(x, a)		(((x) & ((a) - 1)) == 0)

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

void panic(const char *err, ...);

#endif /* UNTITLED_KERNEL_H */
