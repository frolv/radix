/*
 * kernel/mm/buddy.h
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

#ifndef KERNEL_MM_BUDDY_H
#define KERNEL_MM_BUDDY_H

#include <radix/list.h>
#include <radix/mm.h>
#include <radix/mm_types.h>
#include <radix/spinlock.h>

#define BUDDY_INIT {            \
	.len = { 0 },           \
	.max_ord = 0,           \
	.alloc_pages = 0,       \
	.total_pages = 0,       \
	.lock = SPINLOCK_INIT   \
}

struct buddy {
	struct list     ord[PA_ORDERS];         /* lists of 2^i size blocks */
	size_t          len[PA_ORDERS];         /* length of each list */
	size_t          max_ord;                /* maximum available order */
	size_t          total_pages;            /* total pages in this zone */
	size_t          alloc_pages;            /* number of allocated pages */
	spinlock_t      lock;
};

#endif /* KERNEL_MM_BUDDY_H */
