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
 * FFFFFFFFFFFFZARIMCCCCCCCUUUUOOOO
 *
 * OOOO - block order number (first page in block) or PM_PAGE_ORDER_INNER
 * UUUU - maximum order to which pages in block can be coalesced
 * C7   - number of vmm_areas mapping to this page
 * M    - mapped bit. 1: mapped to a virtual address, 0: not mapped
 * I    - invalid bit. 1: not located in valid memory, 0: in valid memory
 * R    - reserved bit. 1: reserved for kernel use, 0: can be allocated
 * A    - allocated bit. 1: allocated, 0: free (only in valid, unreserved pages)
 * Z    - zone bit. 1: user zone, 0: regular zone
 * F12  - offset of page within its maximum block
 */
#define __ORDER_MASK            0x0000000F
#define __MAX_ORDER_MASK        0x000000F0
#define __REFCOUNT_MASK         0x00007F00
#define __OFFSET_MASK           0xFFF00000

#define __MAX_ORDER_SHIFT       4
#define __REFCOUNT_SHIFT        8
#define __OFFSET_SHIFT          20

/*
 * The first page in a block stores the order of the whole block.
 * The rest are assigned the PAGE_ORDER_INNER value.
 */
#define PM_PAGE_ORDER_INNER     0xF
#define PM_PAGE_BLOCK_ORDER(p)  (((p)->status) & __ORDER_MASK)
#define PM_PAGE_MAX_ORDER(p) \
	((((p)->status) & __MAX_ORDER_MASK) >> __MAX_ORDER_SHIFT)
#define PM_PAGE_REFCOUNT(p) \
	((((p)->status) & __REFCOUNT_MASK) >> __REFCOUNT_SHIFT)
#define PM_PAGE_BLOCK_OFFSET(p) \
	((((p)->status) & __OFFSET_MASK) >> __OFFSET_SHIFT)

#define PM_SET_BLOCK_ORDER(p, ord) \
	(p)->status = ((((p)->status) & ~__ORDER_MASK) | (ord))

#define PM_SET_MAX_ORDER(p, ord)                        \
do {                                                    \
	(p)->status &= ~__MAX_ORDER_MASK;               \
	(p)->status |= (ord) << __MAX_ORDER_SHIFT;      \
} while (0)

#define PM_SET_REFCOUNT(p, rc)                          \
do {                                                    \
	(p)->status &= ~__REFCOUNT_MASK;                \
	(p)->status |= (rc) << __REFCOUNT_SHIFT;        \
} while (0)

#define PM_SET_PAGE_OFFSET(p, off)                      \
do {                                                    \
	(p)->status &= ~__OFFSET_MASK;                  \
	(p)->status |= (off) << __OFFSET_SHIFT;         \
} while (0)

#define PAGE_UNINIT_MAGIC       0xDEADFEED

#define PM_PAGE_MAPPED          (1 << 15)
#define PM_PAGE_INVALID         (1 << 16)
#define PM_PAGE_RESERVED        (1 << 17)
#define PM_PAGE_ALLOCATED       (1 << 18)
#define PM_PAGE_ZONE_USR        (1 << 19)

struct page {
	void            *slab_cache;    /* address of slab cache */
	void            *slab_desc;     /* address of slab descriptor */
	void            *mem;           /* start of the page itself */
	unsigned long   status;         /* information about state */
	struct list     list;           /* buddy allocator list */
};

#endif /* RADIX_MM_TYPES_H */
