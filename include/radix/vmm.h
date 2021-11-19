/*
 * include/radix/vmm.h
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

#ifndef RADIX_VMM_H
#define RADIX_VMM_H

#include <radix/asm/mm_limits.h>
#include <radix/list.h>
#include <radix/mm_types.h>
#include <radix/rbtree.h>
#include <radix/spinlock.h>
#include <radix/task.h>
#include <radix/types.h>

struct vmm_area {
    addr_t base;
    size_t size;
    struct list list;
};

struct vmm_structures {
    struct list block_list;    /* all blocks in address space */
    struct list alloc_list;    /* allocated blocks in address space */
    struct rb_root addr_tree;  /* unallocated blocks by address */
    struct rb_root size_tree;  /* unallocated blocks by size */
    struct rb_root alloc_tree; /* allocated blocks by address */
};

struct vmm_space {
    struct vmm_structures structures;
    struct list vmm_list;
    spinlock_t structures_lock;
    paddr_t paging_base;
    int pages;
};

void vmm_init(void);

#define VMM_AREA_MIN_SIZE 64

#define VMM_ALLOC_UPFRONT (1 << 0)

struct vmm_area *vmm_alloc_size(struct vmm_space *vmm,
                                size_t size,
                                unsigned long flags);
void vmm_free(struct vmm_area *area);

void *vmalloc(size_t size);
void vfree(void *ptr);

struct vmm_area *vmm_get_allocated_area(struct vmm_space *vmm, addr_t addr);
void vmm_add_area_pages(struct vmm_area *area, struct page *p);

#endif /* RADIX_VMM_H */
