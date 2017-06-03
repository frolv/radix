/*
 * arch/i386/include/radix/asm/bits.h
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

#ifndef ARCH_I386_RADIX_BITS_H
#define ARCH_I386_RADIX_BITS_H

#ifndef RADIX_BITS_H
#error only <radix/bits.h> can be included directly
#endif

#include <radix/compiler.h>

#include <radix/bits/fls.h>

#define fls(x) __fls(x)
static __always_inline unsigned long __fls(unsigned long x)
{
	if (is_immediate(x))
		return __fls_generic(x);

	asm volatile("bsr %1, %0\n\t"
		     "jnz 1f\n\t"
		     "movl $0, %0\n"
		     "1:"
		     : "=r"(x)
		     : "rm"(x)
		    );

	return x;
}

#endif /* ARCH_I386_RADIX_BITS_H */
