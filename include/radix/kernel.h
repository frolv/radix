/*
 * include/radix/kernel.h
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

#ifndef UNTITLED_KERNEL_H
#define UNTITLED_KERNEL_H

#include <radix/types.h>

#define HALT()  asm volatile("hlt")
#define DIE() \
	do { \
		HALT(); \
	} while (1)

#define ALIGN(x, a)             __ALIGN_MASK(x, (typeof (x))(a) - 1)
#define __ALIGN_MASK(x, mask)   (((x) + (mask)) & ~(mask))

#define ALIGNED(x, a)           (((x) & ((a) - 1)) == 0)

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define SWAP(a, b) \
	do { \
		typeof(a) __tmp = (a); \
		(a) = (b); \
		(b) = __tmp; \
	} while (0)


#include <radix/compiler.h>

#define POW2(x) (1U << (x))

static __always_inline size_t order(size_t n)
{
	size_t ord = 0;

	while ((n >>= 1))
		++ord;

	return ord;
}

#define _K(n)   ((n)   * 1024UL)
#define _M(n)   (_K(n) * 1024UL)
#define _G(n)   (_M(n) * 1024UL)

#include <stdio.h>

#define BOOT_OK_MSG(msg, ...) \
	printf("[ \x1B[1;32mOK\x1B[37m ] " msg, ##__VA_ARGS__)

#define BOOT_FAIL_MSG(msg, ...) \
	printf("[ \x1B[1;34mFAILED\x1B[37m ] " msg, ##__VA_ARGS__)

void panic(const char *err, ...);

#endif /* UNTITLED_KERNEL_H */
