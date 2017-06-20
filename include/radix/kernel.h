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

#ifndef RADIX_KERNEL_H
#define RADIX_KERNEL_H

#define ALIGN(x, a)             __ALIGN_MASK(x, ((typeof (x))(a) - 1))
#define __ALIGN_MASK(x, mask)   (((x) + (mask)) & ~(mask))
#define PTR_ALIGN(p, a)         ((typeof (p))ALIGN((unsigned long)(p), (a)))
#define ALIGNED(x, a)           (((x) & ((typeof (x))(a) - 1)) == 0)
#define ISPOW2(x)               ALIGNED(x, x)

#define FIELD_SIZEOF(t, f)      (sizeof (((t *)0)->f))

#define max(a, b)                       \
({                                      \
	typeof(a) _maxa = (a);          \
	typeof(b) _maxb = (b);          \
	_maxa > _maxb ? _maxa : _maxb;  \
})

#define min(a, b)                       \
({                                      \
	typeof(a) _mina = (a);          \
	typeof(b) _minb = (b);          \
	_mina > _minb ? _mina : _minb;  \
})

#define swap(a, b)                      \
do {                                    \
	typeof(a) __tmp = (a);          \
	(a) = (b);                      \
	(b) = __tmp;                    \
} while (0)

#define KIB(n) ((n)    * 1024UL)
#define MIB(n) (KIB(n) * 1024UL)
#define GIB(n) (MIB(n) * 1024UL)

#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))


#include <radix/asm/halt.h>
#include <radix/compiler.h>

__noreturn void panic(const char *err, ...);

#endif /* RADIX_KERNEL_H */
