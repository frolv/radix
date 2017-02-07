/*
 * include/rlibc/string.h
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

#ifndef RLIBC_STRING_H
#define RLIBC_STRING_H

#include <radix/types.h>

int memcmp(const void *s1, const void *s2, size_t n);
void *memcpy(void *__restrict dst, const void *__restrict src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *s, int c, size_t n);

size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

char *strcpy(char *__restrict dst, const char *__restrict src);
char *strncpy(char *__restrict dst, const char *__restrict src, size_t n);
char *strcat(char *__restrict dst, const char *__restrict src);
char *strncat(char *__restrict dst, const char *__restrict src, size_t n);

const char *strerror(int errno);

char *strrev(char *s);

#endif /* RLIBC_STRING_H */
