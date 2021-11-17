/*
 * lib/rlibc/string/memmove.c
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

#include <rlibc/string.h>

#ifndef __ARCH_HAS_MEMMOVE
void *memmove(void *dst, const void *src, size_t n)
{
	size_t i;
	unsigned char *d = dst;
	const unsigned char *s = src;

	if (d < s) {
		for (i = 0; i < n; ++i)
			d[i] = s[i];
	} else {
		for (i = n; i; --i)
			d[i - 1] = s[i - 1];
	}

	return dst;
}
#endif /* !__ARCH_HAS_MEMMOVE */
