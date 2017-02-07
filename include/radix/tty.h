/*
 * include/radix/tty.h
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

#ifndef UNTITLED_TTY_H
#define UNTITLED_TTY_H

#include <radix/types.h>

#define TTY_TAB_STOP 8

void tty_init(void);
void tty_putchar(int c);
void tty_write(const char *data, size_t size);
void tty_flush(void);

#endif /* UNTITLED_TTY_H */
