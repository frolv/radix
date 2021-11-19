/*
 * lib/rlibc/string/memset.c
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

#include <radix/kernel.h>

#include <rlibc/string.h>

#ifndef __ARCH_HAS_MEMSET

void *memset(void *s, int c, size_t n)
{
    unsigned char *p = s;
#if __WORDSIZE == 32 || __WORDSIZE == 64
    unsigned long *dest;
    unsigned long val, count;

    while (n && !ALIGNED((unsigned long)p, sizeof(unsigned long))) {
        *p++ = c;
        --n;
    }

#if __WORDSIZE == 32
    val = 0x01010101 * (unsigned char)c;
#else
    val = 0x0101010101010101 * (unsigned char)c;
#endif

    dest = (unsigned long *)p;
    count = n / sizeof *dest;
    p += count * sizeof *dest;
    n -= count * sizeof *dest;

    while (count--)
        *dest++ = val;
#endif
    while (n--)
        *p++ = c;

    return s;
}

#endif /* !__ARCH_HAS_MEMSET */
