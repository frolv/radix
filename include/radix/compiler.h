/*
 * include/radix/compiler.h
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

#ifndef RADIX_COMPILER_H
#define RADIX_COMPILER_H

#if defined(__GNUC__) && !defined(__clang__)

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define __always_inline inline __attribute__((always_inline))

#define __noreturn   __attribute__((noreturn))
#define __deprecated __attribute__((deprecated))
#define __unused     __attribute__((unused))
#define __must_check __attribute__((warn_unused_result))
#define __packed     __attribute__((packed))

#define __aligned(x) __attribute__((aligned(x)))
#define __section(x) __attribute__((section(x)))

#define __printf(string_index, arg_index) \
    __attribute__((format(printf, string_index, arg_index)))

#ifndef offsetof
#define offsetof(type, member) __builtin_offsetof(type, member)
#endif  // offsetof

#define container_of(ptr, type, member)                   \
    ({                                                    \
        const typeof(((type *)0)->member) *__ptr = (ptr); \
        (type *)((char *)__ptr - offsetof(type, member)); \
    })

#define is_immediate(exp) (__builtin_constant_p(exp))

#define barrier() asm volatile("" : : : "memory")

#else

#error Only gcc is supported at this time

#endif  // defined(__GNUC__) && !defined(__clang__)

#endif  // RADIX_COMPILER_H
