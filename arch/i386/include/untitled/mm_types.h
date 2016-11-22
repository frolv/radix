/*
 * arch/i386/include/untitled/mm_types.h
 * Copyright (C) 2016 Alexei Frolov
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

#ifndef ARCH_I386_UNTITLED_MM_TYPES_H
#define ARCH_I386_UNTITLED_MM_TYPES_H

#define __ARCH_KERNEL_VIRT_BASE 0xC0000000UL

typedef unsigned long addr_t;

typedef unsigned long pdeval_t;
typedef unsigned long pteval_t;

typedef struct {
	pdeval_t pde;
} pde_t;

typedef struct {
	pteval_t pte;
} pte_t;

#include <untitled/list.h>

#define __ARCH_INNER_ORDER	0xF
#define __PAGE_BLOCK_ORDER(p)	(((p)->status) & 0x0000000F)
#define ST_PAGE_MAPPED		(1 << 8)
#define ST_PAGE_INVALID		(1 << 9)
#define ST_PAGE_RESERVED	(1 << 10)

struct page {
	void		*slab_cache;	/* address of slab cache */
	void		*slab_desc;	/* address of slab descriptor */
	void		*mem;		/* start of the page itself */
	unsigned int	status;		/* information about state */
	struct list	list;		/* buddy allocator list */
};

#endif /* ARCH_I386_UNTITLED_MM_TYPES_H */
