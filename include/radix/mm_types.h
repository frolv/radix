/*
 * include/radix/mm_types.h
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

#ifndef RADIX_MM_TYPES_H
#define RADIX_MM_TYPES_H

#include <radix/asm/mm_types.h>
#include <radix/list.h>

/*
 * page status (32-bit):
 * FFFFFFFFFFFFFFFFxxxZARIMUUUUOOOO
 *
 * x    - currently unused
 * OOOO - block order number (first page in block) or PM_PAGE_ORDER_INNER
 * UUUU - maximum order to which pages in block can be coalesced
 * M    - mapped bit. 1: mapped to a virtual address, 0: not mapped
 * I    - invalid bit. 1: not located in valid memory, 0: in valid memory
 * R    - reserved bit. 1: reserved for kernel use, 0: can be allocated
 * A    - allocated bit. 1: allocated, 0: free (only in valid, unreserved pages)
 * Z    - zone bit. 1: user zone, 0: regular zone
 * F    - offset of page within its maximum block
 */
#define __ORDER_MASK            0x0000000F
#define __MAX_ORDER_MASK        0x000000F0
#define __OFFSET_MASK           0xFFFF0000

/*
 * The first page in a block stores the order of the whole block.
 * The rest are assigned the PAGE_ORDER_INNER value.
 */
#define PM_PAGE_ORDER_INNER     0xF
#define PM_PAGE_BLOCK_ORDER(p)  (((p)->status) & __ORDER_MASK)
#define PM_PAGE_MAX_ORDER(p)    ((((p)->status) & __MAX_ORDER_MASK) >> 4)

#define PM_PAGE_BLOCK_OFFSET(p) ((((p)->status) & __OFFSET_MASK) >> 16)
#define PM_SET_BLOCK_ORDER(p, ord) \
	(p)->status = ((((p)->status) & ~__ORDER_MASK) | (ord))
#define PM_SET_MAX_ORDER(p, ord) \
	(p)->status = ((((p)->status) & ~__MAX_ORDER_MASK) | ((ord) << 4))
#define PM_SET_PAGE_OFFSET(p, off) \
	(p)->status = ((((p)->status) & ~__OFFSET_MASK) | ((off) << 16))

#define PAGE_UNINIT_MAGIC       0xDEADFEED

#define PM_PAGE_MAPPED          (1 << 8)
#define PM_PAGE_INVALID         (1 << 9)
#define PM_PAGE_RESERVED        (1 << 10)
#define PM_PAGE_ALLOCATED       (1 << 11)
#define PM_PAGE_ZONE_USR        (1 << 12)

struct page {
	void            *slab_cache;    /* address of slab cache */
	void            *slab_desc;     /* address of slab descriptor */
	void            *mem;           /* start of the page itself */
	unsigned long   status;         /* information about state */
	struct list     list;           /* buddy allocator list */
};

#endif /* RADIX_MM_TYPES_H */
