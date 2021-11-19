/*
 * include/rlibc/ctype.h
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

#ifndef RLIBC_CTYPE_H
#define RLIBC_CTYPE_H

#include <radix/compiler.h>

static __always_inline int isspace(int c)
{
    return c == ' ' || c == '\n' || c == '\t' || c == '\v' || c == '\f' ||
           c == '\r';
}

static __always_inline int isupper(int c) { return c >= 'A' && c <= 'Z'; }

static __always_inline int islower(int c) { return c >= 'a' && c <= 'z'; }

static __always_inline int isdigit(int c) { return c >= '0' && c <= '9'; }

static __always_inline int isalpha(int c) { return islower(c) || isupper(c); }

static __always_inline int isalnum(int c) { return isalpha(c) || isdigit(c); }

static __always_inline int tolower(int c)
{
    return isupper(c) ? c ^ (1 << 5) : c;
}

static __always_inline int toupper(int c)
{
    return islower(c) ? c ^ (1 << 5) : c;
}

#endif /* RLIBC_CTYPE_H */
