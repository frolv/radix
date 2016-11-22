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

#include <string.h>
#include <untitled/kernel.h>
#include <untitled/mm.h>
#include <untitled/page.h>

#include "buddy.h"

#define PAGE_UNINIT_MAGIC	0xDEADFEED

struct page *page_map = (struct page *)PAGE_MAP_BASE;
addr_t page_map_end = PAGE_MAP_BASE;

/* First 16 MiB of memory. */
static struct buddy zone_dma;
/* The rest of memory. */
static struct buddy zone_reg;

/* total amount of usable memory in the system */
uint64_t totalmem = 0;

static int next_phys_region(struct multiboot_info *mbt,
			    uint64_t *base, uint64_t *len);
static void init_region(addr_t base, uint64_t len, unsigned int flags);

void buddy_init(struct multiboot_info *mbt)
{
	uint64_t base, len, next;

	/*
	 * mmap_addr stores the physical address of the memory map.
	 * Make it virtual.
	 */
	mbt->mmap_addr += KERNEL_VIRTUAL_BASE;

	next = 0;
	while (next_phys_region(mbt, &base, &len)) {
		/*
		 * next_phys_region only returns valid memory regions,
		 * but all existing memory needs to be mapped.
		 */
		if (base != next)
			init_region(next, base - next, ST_PAGE_INVALID);

		init_region(base, len, 0);
		totalmem += len;
		next = base + len;
	}
}

static struct page *__alloc_pages(struct buddy *zone, size_t ord);

/*
 * Allocate a contiguous block of pages in memory.
 * Behaviour of the allocator is managed by flags.
 */
struct page *alloc_pages(unsigned int flags, size_t ord)
{
	if (ord > PA_MAX_ORDER - 1)
		return ERR_PTR(EINVAL);

	if (flags & PA_ZONE_DMA)
		return __alloc_pages(&zone_dma, ord);
	else
		return __alloc_pages(&zone_reg, ord);
}

void free_page(void *base)
{
	struct page *p;
	addr_t virt;

	virt = (addr_t)base;
	if (!ALIGNED(virt, PAGE_SIZE))
		return;

	p = virt_to_page(base);
	p->slab_cache = (void *)PAGE_UNINIT_MAGIC;
	p->slab_desc = (void *)PAGE_UNINIT_MAGIC;
	p->mem = (void *)PAGE_UNINIT_MAGIC;
}

/* Allocate 2^{ord} pages from zone. */
static struct page *__alloc_pages(struct buddy *zone, size_t ord)
{
	if (zone->max_ord > ord)
		return ERR_PTR(ENOMEM);

	return NULL;
}

static struct memory_map *mmap = NULL;

#define NEXT_MAP(mmap) \
	((struct memory_map *)((addr_t)mmap + mmap->size + sizeof (mmap->size)))

#define IN_RANGE(mmap, mbt) \
	((addr_t)mmap < mbt->mmap_addr + mbt->mmap_length)

#define make64(low, high) ((uint64_t)(high) << 32 | (low))

/*
 * Find the next physical region in the multiboot memory map.
 * Store its base address and length.
 * Return 0 when all memory has been read.
 */
static int next_phys_region(struct multiboot_info *mbt,
			    uint64_t *base, uint64_t *len)
{
	uint64_t b, l, orig_base;

	if (!mmap)
		mmap = (struct memory_map *)mbt->mmap_addr;
	else
		mmap = NEXT_MAP(mmap);

	/* only consider available RAM */
	while (mmap->type != 1 && IN_RANGE(mmap, mbt))
		mmap = NEXT_MAP(mmap);

	if (!IN_RANGE(mmap, mbt))
		return 0;

	b = make64(mmap->base_addr_low, mmap->base_addr_high);
	l = make64(mmap->length_low, mmap->length_high);

	/*
	 * This should already be aligned by the bootloader.
	 * But, just in case...
	 */
	orig_base = b;
	b = ALIGN(b, PAGE_SIZE);
	l = (l - (b - orig_base)) & PAGE_MASK;

	*base = b;
	*len = l;
	return 1;
}

static size_t order(size_t n)
{
	size_t ord = 0;

	while ((n >>= 1))
		++ord;

	return ord;
}

/*
 * Allocate required page tables for the page map
 * going backwards from the end of the kernel.
 */
static addr_t curr_pgtbl = KERNEL_VIRTUAL_BASE + KERNEL_SIZE - PGTBL_SIZE;
static size_t ntables = 0;

/* Number of pages used for the page_map */
static size_t npages = 0;

#define T(x) (1U << (x))

static void check_space(size_t pfn);

/* Populate struct pages for a region of physical memory starting at base. */
static void init_region(addr_t base, uint64_t len, unsigned int flags)
{
	size_t ord, pfn, start, pages;
	addr_t end;

	while (len) {
		pages = len / PAGE_SIZE;

		/* determine the size of the block, up the the maximum 2^9 */
		if ((ord = order(pages)) > PA_MAX_ORDER - 1)
			ord = PA_MAX_ORDER - 1;
		if (pages < T(ord))
			--ord;

		end = base + T(ord) * PAGE_SIZE;

		/* initialize all pages in the block */
		start = base >> PAGE_SHIFT;
		for (; base < end; base += PAGE_SIZE) {
			pfn = base >> PAGE_SHIFT;

			check_space(pfn);

			page_map[pfn].slab_cache = (void *)PAGE_UNINIT_MAGIC;
			page_map[pfn].slab_desc = (void *)PAGE_UNINIT_MAGIC;
			page_map[pfn].mem = (void *)PAGE_UNINIT_MAGIC;
			page_map[pfn].status = ST_PAGE_ORDER_INNER | flags;
			list_init(&page_map[pfn].list);

			len -= PAGE_SIZE;
		}
		page_map[start].status = ord | flags;
	}
}

/* Ensure that page table is set up for page map entries. */
static void check_space(size_t pfn)
{
	size_t req_len, off, tbl;
	const unsigned int flags = PAGE_RW | PAGE_PRESENT;

	req_len = (pfn + 1) * sizeof (struct page);

	/* Check if a new virtual page needs to be mapped */
	if (req_len > npages * PAGE_SIZE) {
		/* Check if a new page table is required */
		tbl = ntables * PAGE_SIZE * PTRS_PER_PGTBL;
		if (req_len > tbl) {
			memset((void *)curr_pgtbl, 0, PGTBL_SIZE);
			__create_pgtbl(PAGE_MAP_BASE + tbl,
				       make_pde(phys_addr(curr_pgtbl) | flags));
			curr_pgtbl -= PGTBL_SIZE;
			++ntables;
		}

		off = npages * PAGE_SIZE;
		map_page(PAGE_MAP_BASE + off, __PAGE_MAP_PHYS_BASE + off);
		++npages;
		page_map_end += PAGE_SIZE;
	}
}
