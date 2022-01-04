/*
 * include/radix/tty.h
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

#ifndef RADIX_TTY_H
#define RADIX_TTY_H

#include <stddef.h>

#define TTY_TAB_STOP 8

// Writes a string to the TTY. Null terminators within the string are ignored;
// the size argument determines the number of bytes that will be written.
void tty_write(const char *data, size_t size);

static inline void tty_putchar(int c)
{
    char ch = c;
    tty_write(&ch, sizeof ch);
}

void tty_flush(void);

#endif /* RADIX_TTY_H */
