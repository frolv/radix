/*
 * include/radix/vmm.h
 * Copyright (C) 2021 Alexei Frolov
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
#include <radix/error.h>
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
    // All blocks in the address space.
    struct list block_list;

    // Allocated blocks in the address space.
    struct list alloc_list;

    // Unallocated blocks sorted by base address.
    struct rb_root addr_tree;

    // Unallocated blocks sorted by size.
    struct rb_root size_tree;

    // Allocated blocks sorted by base address.
    struct rb_root alloc_tree;
};

struct vmm_space {
    struct vmm_structures structures;
    struct list vmm_list;
    spinlock_t lock;
    paddr_t paging_base;
    void *paging_ctx;
    int pages;
};

// Initializes the virtual memory management system.
void vmm_init(void);

// Creates a new vmm_space for a process.
struct vmm_space *vmm_new(void);

// Releases a vmm_space.
void vmm_release(struct vmm_space *vmm);

// Returns the kernel's address space.
struct vmm_space *vmm_kernel(void);

#define VMM_READ          (1 << 0)
#define VMM_WRITE         (1 << 1)
#define VMM_EXEC          (1 << 2)
#define VMM_ALLOC_UPFRONT (1 << 8)

struct vmm_area *vmm_alloc_addr(struct vmm_space *vmm,
                                addr_t addr,
                                size_t size,
                                uint32_t flags);
struct vmm_area *vmm_alloc_size(struct vmm_space *vmm,
                                size_t size,
                                uint32_t flags);
void vmm_free(struct vmm_area *area);

void *vmalloc(size_t size);
void vfree(void *ptr);

struct vmm_area *vmm_get_allocated_area(struct vmm_space *vmm, addr_t addr);

// Marks a block of physical pages as allocated for a VMM area. This does not
// map the pages to addresses in the area; that must be done separately.
void vmm_add_area_pages(struct vmm_area *area, struct page *p);

// Maps physical pages to an address within an allocated VMM area.
int vmm_map_pages(struct vmm_area *area, addr_t addr, struct page *p);

void vmm_space_dump(struct vmm_space *vmm);

//
// Architecture-specific functions.
//

// Initializes the virtual memory management system.
void arch_vmm_init(struct vmm_space *kernel_vmm_space);

// Initializes an address space for a process.
int arch_vmm_setup(struct vmm_space *vmm);

// Frees an address space.
void arch_vmm_release(struct vmm_space *vmm);

#endif  // RADIX_VMM_H
