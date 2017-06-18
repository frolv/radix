/*
 * include/radix/compiler.h
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

#ifndef RADIX_COMPILER_H
#define RADIX_COMPILER_H

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

#define __always_inline inline __attribute__((always_inline))

#define __noreturn __attribute__((noreturn))
#define __deprecated __attribute__((deprecated))

#define __aligned(x) __attribute__((aligned(x)))
#define __section(x) __attribute__((section(x)))

#define offsetof(type, member) __builtin_offsetof(type, member)

#define container_of(ptr, type, member)                         \
({                                                              \
	const typeof(((type *)0)->member) *__ptr = (ptr);       \
	(type *)((char *)__ptr - offsetof(type, member));       \
})

#define is_immediate(exp) (__builtin_constant_p(exp))

#define barrier() asm volatile("" : : : "memory")

#endif /* RADIX_COMPILER_H */
