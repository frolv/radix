/*
 * arch/i386/include/radix/asm/limits.h
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

#ifndef ARCH_I386_RADIX_LIMITS_H
#define ARCH_I386_RADIX_LIMITS_H

#ifndef RADIX_LIMITS_H
#error only <radix/limits.h> can be included directly
#endif

#define __CHAR_BIT 8

#define __SCHAR_MAX 127
#define __SCHAR_MIN (-__SCHAR_MAX - 1)

#define __UCHAR_MAX (__SCHAR_MAX * 2U + 1)

#ifdef __CHAR_UNSIGNED__
#define __CHAR_MIN 0
#define __CHAR_MAX __UCHAR_MAX
#else
#define __CHAR_MIN __SCHAR_MIN
#define __CHAR_MAX __SCHAR_MAX
#endif

#define __SHRT_MAX 32767
#define __SHRT_MIN (-__SHRT_MAX - 1)

#define __USHRT_MAX (__SHRT_MAX * 2U + 1)

#define __INT_MAX 2147483647
#define __INT_MIN (-__INT_MAX - 1)

#define __UINT_MAX (__INT_MAX * 2U + 1)

#define __LONG_MAX 2147483647L
#define __LONG_MIN (-__LONG_MAX - 1)

#define __ULONG_MAX (__LONG_MAX * 2UL + 1)

#define __LONG_LONG_MAX 9223372036854775807LL
#define __LONG_LONG_MIN (-__LONG_LONG_MAX - 1)

#define __ULONG_LONG_MAX (__LONG_LONG_MAX * 2ULL + 1)

#endif /* ARCH_I386_RADIX_LIMITS_H */
