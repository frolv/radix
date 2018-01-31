/*
 * arch/i386/include/radix/asm/bits.h
 * Copyright (C) 2016-2018 Alexei Frolov
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
#include <radix/types.h>

#include <radix/bits/ffs.h>
#include <radix/bits/fls.h>

#define __fop_size(name, x)                                             \
({                                                                      \
	unsigned int __fop_ret;                                         \
	switch (sizeof (x)) {                                           \
	case 1: case 2: case 4: __fop_ret = __##name##_32(x); break;    \
	case 8: __fop_ret = __##name##_64(x); break;                    \
	}                                                               \
	__fop_ret;                                                      \
})

#define __fop(name, x) \
	(!is_immediate(x) ? __##name##_generic(x) : __fop_size(name, x))

#define ffs(x) __fop(ffs, x)
#define fls(x) __fop(fls, x)

static __always_inline unsigned int __ffs_32(uint32_t x)
{
	int r;

	asm volatile("bsfl %1, %0\n\t"
	             "jnz 1f\n\t"
	             "movl $-1, %0\n"
	             "1:"
	             : "=r"(r)
	             : "rm"(x)
	            );

	return r + 1;
}

static __always_inline unsigned int __ffs_64(uint32_t x)
{
#ifdef CONFIG_X86_64
	int pos = -1;

	asm volatile("bsfq %1, %q0"
	             : "+r"(pos)
	             : "rm"(x));
	return pos + 1;
#else
	return __ffs_generic(x);
#endif /* CONFIG_X86_64 */
}

static __always_inline unsigned int __fls_32(uint32_t x)
{
	int r;

	asm volatile("bsrl %1, %0\n\t"
	             "jnz 1f\n\t"
	             "movl $-1, %0\n"
	             "1:"
	             : "=r"(r)
	             : "rm"(x)
	            );

	return r + 1;
}

static __always_inline unsigned int __fls_64(uint64_t x)
{
#ifdef CONFIG_X86_64
	int pos = -1;

	asm volatile("bsrq %1, %q0"
	             : "+r"(pos)
	             : "rm"(x));
	return pos + 1;
#else
	return __fls_generic(x);
#endif /* CONFIG_X86_64 */
}

#endif /* ARCH_I386_RADIX_BITS_H */
