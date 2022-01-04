/*
 * include/radix/atomic.h
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

#ifndef RADIX_ATOMIC_H
#define RADIX_ATOMIC_H

#include <radix/asm/atomic.h>
#include <radix/irqstate.h>

#define atomic_swap(p, val)         __atomic_by_size_ret(swap, p, val)
#define atomic_cmpxchg(p, old, new) __atomic_by_size_ret(cmpxchg, p, old, new)

#define atomic_write(p, val) __atomic_by_size(write, p, val)
#define atomic_read(p)       __atomic_by_size_ret(read, p)

#define atomic_or(p, val)  __atomic_by_size(or, p, val)
#define atomic_and(p, val) __atomic_by_size(and, p, val)
#define atomic_add(p, val) __atomic_by_size(add, p, val)
#define atomic_sub(p, val) __atomic_by_size(sub, p, val)
#define atomic_inc(p)      atomic_add(p, 1)
#define atomic_dec(p)      atomic_sub(p, 1)

#define atomic_fetch_add(p, val) __atomic_by_size_ret(fetch_add, p, val)
#define atomic_fetch_inc(p)      atomic_fetch_add(p, 1)

void atomic_bad_size_call(void);

#define __atomic_by_size_ret(op, p, ...)                   \
    ({                                                     \
        typeof(*(p)) __abs_ret;                            \
        switch (sizeof(*(p))) {                            \
        case 1:                                            \
            __abs_ret = atomic_##op##_1(p, ##__VA_ARGS__); \
            break;                                         \
        case 2:                                            \
            __abs_ret = atomic_##op##_2(p, ##__VA_ARGS__); \
            break;                                         \
        case 4:                                            \
            __abs_ret = atomic_##op##_4(p, ##__VA_ARGS__); \
            break;                                         \
        case 8:                                            \
            __abs_ret = atomic_##op##_8(p, ##__VA_ARGS__); \
            break;                                         \
        default:                                           \
            atomic_bad_size_call();                        \
            break;                                         \
        }                                                  \
        __abs_ret;                                         \
    })

#define __atomic_by_size(op, p, ...)           \
    do {                                       \
        switch (sizeof(*(p))) {                \
        case 1:                                \
            atomic_##op##_1(p, ##__VA_ARGS__); \
            break;                             \
        case 2:                                \
            atomic_##op##_2(p, ##__VA_ARGS__); \
            break;                             \
        case 4:                                \
            atomic_##op##_4(p, ##__VA_ARGS__); \
            break;                             \
        case 8:                                \
            atomic_##op##_8(p, ##__VA_ARGS__); \
            break;                             \
        default:                               \
            atomic_bad_size_call();            \
            break;                             \
        }                                      \
    } while (0)

#endif  // RADIX_ATOMIC_H
