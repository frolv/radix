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

#define __PAGE_MAP_PHYS_BASE	0x01000000
#define PAGE_MAP_BASE		(KERNEL_VIRTUAL_BASE + __PAGE_MAP_PHYS_BASE)

extern uint64_t totalmem;

void page_map_init(void);
void detect_memory(multiboot_info_t *mbt);

struct page *alloc_page(void);
void free_page(void *base);

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
