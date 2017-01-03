/*
 * include/stdio.h
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

#ifndef UNTITLED_STDIO_H
#define UNTITLED_STDIO_H

#include <stdarg.h>
#include <untitled/types.h>

#define EOF (-1)

int putchar(int c);
int puts(const char *s);

int printf(const char *format, ...);
int vprintf(const char *format, va_list ap);

int sprintf(char *str, const char *format, ...);
int vsprintf(char *str, const char *format, va_list ap);

int snprintf(char *str, size_t size, const char *format, ...);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);

#endif /* UNTITLED_STDIO_H */
