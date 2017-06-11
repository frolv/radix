/*
 * include/radix/vmm.h
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

#ifndef RADIX_VMM_H
#define RADIX_VMM_H

#include <radix/asm/mm_limits.h>
#include <radix/list.h>
#include <radix/mm_types.h>
#include <radix/rbtree.h>

struct vmm_structures {
	struct list    block_list;
	struct rb_root addr_tree;
	struct rb_root size_tree;
};

void vmm_init(void);

#endif /* RADIX_VMM_H */
