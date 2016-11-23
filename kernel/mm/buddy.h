/*
 * kernel/mm/page.c
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

#ifndef KERNEL_MM_BUDDY_H
#define KERNEL_MM_BUDDY_H

#include <untitled/list.h>
#include <untitled/mm.h>
#include <untitled/mm_types.h>

/*
 * The first page in a block stores the order of the whole block.
 * The rest are assigned the PAGE_ORDER_INNER value.
 */
#define PM_PAGE_ORDER_INNER		__ARCH_INNER_ORDER
#define PM_PAGE_BLOCK_ORDER(p)		__PAGE_BLOCK_ORDER(p)
#define PM_PAGE_MAX_ORDER(p)		__PAGE_MAX_ORDER(p)
#define PM_SET_BLOCK_ORDER(p, ord)	__SET_BLOCK_ORDER(p, ord)
#define PM_SET_MAX_ORDER(p, ord)	__SET_MAX_ORDER(p, ord)

struct buddy {
	struct list	ord[PA_MAX_ORDER];	/* lists of 2^i size blocks */
	size_t		len[PA_MAX_ORDER];	/* length of each list */
	size_t		max_ord;		/* maximum available order */
};

#endif /* KERNEL_MM_BUDDY_H */
