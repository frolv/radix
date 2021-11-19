/*
 * include/radix/bits/fls.h
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

#ifndef RADIX_BITS_FLS_H
#define RADIX_BITS_FLS_H

#ifndef RADIX_BITS_H
#error only <radix/bits.h> can be included directly
#endif

#include <radix/compiler.h>
#include <radix/types.h>

static __always_inline unsigned int __fls_generic(uint64_t x)
{
    int ord = 1;

    if (!x)
        return 0;

    while ((x >>= 1))
        ++ord;

    return ord;
}

#endif /* RADIX_BITS_FLS_H */
