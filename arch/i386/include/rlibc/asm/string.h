/*
 * arch/i386/include/rlibc/asm/string.h
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

#ifndef ARCH_I386_RLIBC_STRING_H
#define ARCH_I386_RLIBC_STRING_H

#ifndef RLIBC_STRING_H
#error only <rlibc/string.h> can be included directly
#endif

#include <radix/compiler.h>

#define __ARCH_HAS_MEMSET
static __always_inline void *memset(void *s, int c, size_t n)
{
	int a, b;

	asm volatile("rep\n\t"
	             "stosb"
	             : "=&c"(a), "=&D"(b)
	             : "a"(c), "1"(s), "0"(n)
	             : "memory");

	return s;
}

#endif /* ARCH_I386_RLIBC_STRING_H */
