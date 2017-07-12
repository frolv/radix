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

#define __ARCH_HAS_MEMCPY
static __always_inline void *memcpy(void *dst, const void *src, size_t n)
{
	int a, b, c;

	asm volatile("rep\n\t"
	             "movsl\n\t"
	             "movl %4, %%ecx\n\t"
	             "andl $3, %%ecx\n\t"
	             "jz 1f\n\t"
	             "rep\n\t"
	             "movsb\n\t"
	             "1:"
	             : "=&c"(a), "=&D"(b), "=&S"(c)
	             : "0"(n / sizeof (long)), "g"(n), "1"(dst), "2"(src)
	             : "memory");

	return dst;
}

#define __ARCH_HAS_MEMCHR
static __always_inline void *memchr(void *s, int c, size_t n)
{
	void *ret;
	int a;

	if (!n)
		return NULL;

	asm volatile("repne\n\t"
	             "scasb\n\t"
	             "je 1f\n\t"
	             "movl $1, %0\n"
	             "1:\tdecl %0"
	             : "=D"(ret), "=&c"(a)
	             : "a"(c), "0"(s), "1"(n)
	             : "memory");

	return ret;
}

#endif /* ARCH_I386_RLIBC_STRING_H */
