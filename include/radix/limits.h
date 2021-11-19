/*
 * include/radix/limits.h
 * Copyright (C) 2017 Alexei Frolov
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

#ifndef RADIX_LIMITS_H
#define RADIX_LIMITS_H

#include <radix/asm/limits.h>

#define CHAR_BIT __CHAR_BIT

#define SCHAR_MIN __SCHAR_MIN
#define SCHAR_MAX __SCHAR_MAX
#define UCHAR_MAX __UCHAR_MAX

#define CHAR_MIN __CHAR_MIN
#define CHAR_MAX __CHAR_MAX

#define SHRT_MIN  __SHRT_MIN
#define SHRT_MAX  __SHRT_MAX
#define USHRT_MAX __USHRT_MAX

#define INT_MIN  __INT_MIN
#define INT_MAX  __INT_MAX
#define UINT_MAX __UINT_MAX

#define LONG_MIN  __LONG_MIN
#define LONG_MAX  __LONG_MAX
#define ULONG_MAX __ULONG_MAX

#define LONG_LONG_MIN  __LONG_LONG_MIN
#define LONG_LONG_MAX  __LONG_LONG_MAX
#define ULONG_LONG_MAX __ULONG_LONG_MAX

#endif /* RADIX_LIMITS_H */
