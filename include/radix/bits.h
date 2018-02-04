/*
 * include/radix/bits.h
 * Copyright (C) 2016-2018 Alexei Frolov
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

#ifndef RADIX_BITS_H
#define RADIX_BITS_H

#include <radix/asm/bits.h>
#include <radix/compiler.h>

#ifndef ffs
#include <radix/bits/ffs.h>

#define ffs(x) __ffs_generic(x)
#endif

#ifndef fls
#include <radix/bits/fls.h>

#define fls(x) __fls_generic(x)
#endif

static __always_inline unsigned int fns(uint64_t x, unsigned int bit)
{
	int ret;

	if (!bit)
		return 0;

	ret = ffs(x >> bit);
	return ret ? bit + ret : 0;
}

#define pow2(x) (1U << (x))
#define log2(x) (fls(x) - 1)

#endif /* RADIX_BITS_H */
