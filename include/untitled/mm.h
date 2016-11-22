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

#include <stddef.h>
#include <stdint.h>
#include <untitled/error.h>
#include <untitled/mm_types.h>
#include <untitled/multiboot.h>
#include <untitled/page.h>

#define KERNEL_VIRTUAL_BASE	__ARCH_KERNEL_VIRT_BASE
#define KERNEL_SIZE		0x00400000

/*
 * Page map starts at 16 MiB in physical memory, directly after the DMA zone.
 */
#define __PAGE_MAP_PHYS_BASE	0x01000000
#define PAGE_MAP_BASE		(KERNEL_VIRTUAL_BASE + __PAGE_MAP_PHYS_BASE)

extern uint64_t totalmem;

void buddy_init(struct multiboot_info *mbt);


/*
 * Page allocation order.
 * The maximum amount of pages that can be allocated
 * at a time is 2^(PAGE_MAX_ORDER - 1).
 */
#define PA_MAX_ORDER 10

/* Page allocation flags */
#define PA_STANDARD	0x0
#define PA_ZONE_DMA	0x1

struct page *alloc_pages(unsigned int flags, size_t ord);
void free_page(void *base);

static __always_inline struct page *alloc_page(unsigned int flags)
{
	return alloc_pages(flags, 0);
}


#define phys_addr(x) __pa((addr_t)x)

extern struct page *page_map;

/* Find the struct page that corresponds to an address. */
static __always_inline struct page *virt_to_page(void *ptr)
{
	return page_map + (phys_addr(ptr) >> PAGE_SHIFT);
}

int map_page(addr_t virt, addr_t phys);
int unmap_page(addr_t virt);
int unmap_page_pgdir(addr_t virt);

#endif /* UNTITLED_MM_H */
