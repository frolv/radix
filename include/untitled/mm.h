/*
 * include/untitled/mm.h
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

#ifndef UNTITLED_MM_H
#define UNTITLED_MM_H

#include <untitled/error.h>
#include <untitled/mm_types.h>
#include <untitled/multiboot.h>
#include <untitled/page.h>
#include <untitled/types.h>

#define KERNEL_VIRTUAL_BASE     __ARCH_KERNEL_VIRT_BASE
#define KERNEL_SIZE             0x00400000

/*
 * Page map starts at 16 MiB in physical memory, directly after the DMA zone.
 */
#define __PAGE_MAP_PHYS_BASE    0x01000000
#define PAGE_MAP_BASE           (KERNEL_VIRTUAL_BASE + __PAGE_MAP_PHYS_BASE)


#define MEM_LIMIT __ARCH_MEM_LIMIT

extern uint64_t totalmem;

void buddy_init(struct multiboot_info *mbt);


/*
 * Page allocation order.
 * The maximum amount of pages that can be allocated
 * at a time is 2^(PAGE_MAX_ORDER - 1).
 */
#define PA_MAX_ORDER 10

/* Low level page allocation flags */
#define __PA_ZONE_REG   0x0     /* allocate from regular (kernel) zone */
#define __PA_ZONE_DMA   0x1     /* allocate from DMA zone */
#define __PA_ZONE_USR   0x2     /* allocate from user zone */
#define __PA_NO_MAP     0x4     /* do not map pages to a virtual address */

/* Page allocation flags */
#define PA_STANDARD     (__PA_ZONE_REG)
#define PA_DMA          (__PA_ZONE_DMA | __PA_NO_MAP)
#define PA_USER         (__PA_ZONE_USR | __PA_NO_MAP)
#define PA_PAGETABLE    (__PA_ZONE_REG | __PA_NO_MAP)

#define PAGE_UNINIT_MAGIC       0xDEADFEED

/*
 * The first page in a block stores the order of the whole block.
 * The rest are assigned the PAGE_ORDER_INNER value.
 */
#define PM_PAGE_ORDER_INNER             __ARCH_INNER_ORDER
#define PM_PAGE_BLOCK_ORDER(p)          __PAGE_BLOCK_ORDER(p)
#define PM_PAGE_MAX_ORDER(p)            __PAGE_MAX_ORDER(p)

struct page *alloc_pages(unsigned int flags, size_t ord);
void free_pages(struct page *p);

static __always_inline struct page *alloc_page(unsigned int flags)
{
	return alloc_pages(flags, 0);
}


#define phys_addr(x) __pa((addr_t)x)

extern struct page *page_map;

#define PFN(x) (phys_addr(x) >> PAGE_SHIFT)

/* Find the struct page that corresponds to an address. */
static __always_inline struct page *virt_to_page(void *ptr)
{
	return page_map + PFN(ptr);
}

static __always_inline size_t page_to_pfn(struct page *p)
{
	return p - page_map;
}

/* Find the physical address represented by a struct page. */
static __always_inline addr_t page_to_phys(struct page *p)
{
	return page_to_pfn(p) << PAGE_SHIFT;
}

void __create_pgtbl(addr_t virt, pde_t pde);
int map_page(addr_t virt, addr_t phys);
int map_pages(addr_t virt, addr_t phys, size_t n);
int unmap_page(addr_t virt);
int unmap_page_pgdir(addr_t virt);

#endif /* UNTITLED_MM_H */
