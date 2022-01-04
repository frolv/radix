/*
 * arch/i386/stacktrace.c
 * Copyright (C) 2018 Alexei Frolov
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

#include <radix/stacktrace.h>

#include <rlibc/stdio.h>

#include <stdint.h>

#ifdef DEBUG_STACKTRACE

int stack_trace(char *buf, size_t size)
{
    uint32_t *ebp, *eip;
    char *s;
    int i, n;

    asm volatile("movl %%ebp, %0" : "=r"(ebp));

    --size;
    n = snprintf(buf, size, "\nstack trace:\n");
    s = buf + n;
    size -= n;

    /*
     * This will terminate properly as the initial value of ebp is always
     * set to 0 when a processor starts, or when a process is created.
     *
     * TODO: replace the null with a symbol lookup once those are supported.
     */
    for (i = 0; ebp && size && i < STACKTRACE_DEPTH; ++i) {
        eip = ebp + 1;
        if (*eip) {
            n = snprintf(s, size, "\t[%p] null\n", *eip);
            s += n;
            size -= n;
        }
        ebp = (uint32_t *)*ebp;
    }
    *s++ = '\n';

    return s - buf;
}

#endif /* DEBUG_STACKTRACE */
