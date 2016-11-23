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
static void buddy_populate(void);

void buddy_init(struct multiboot_info *mbt)
{
	uint64_t base, len, next;
	size_t i;

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
			init_region(next, base - next, PM_PAGE_INVALID);

		init_region(base, len, 0);
		next = base + len;
	}

	for (i = 0; i < PA_MAX_ORDER; ++i) {
		list_init(&zone_dma.ord[i]);
		zone_dma.len[i] = 0;
		list_init(&zone_reg.ord[i]);
		zone_reg.len[i] = 0;
	}
	buddy_populate();
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
	while (mmap->type != 1 && IN_RANGE(mmap, mbt)) {
		totalmem += make64(mmap->length_low, mmap->length_high);
		mmap = NEXT_MAP(mmap);
	}

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
	totalmem += l;
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
		if (pages < TWO(ord))
			--ord;

		end = base + TWO(ord) * PAGE_SIZE;

		/* initialize all pages in the block */
		start = base >> PAGE_SHIFT;
		for (; base < end; base += PAGE_SIZE) {
			pfn = base >> PAGE_SHIFT;

			check_space(pfn);

			page_map[pfn].slab_cache = (void *)PAGE_UNINIT_MAGIC;
			page_map[pfn].slab_desc = (void *)PAGE_UNINIT_MAGIC;
			page_map[pfn].mem = (void *)PAGE_UNINIT_MAGIC;
			page_map[pfn].status = PM_PAGE_ORDER_INNER | flags;
			list_init(&page_map[pfn].list);

			len -= PAGE_SIZE;
		}
		page_map[start].status = ord | flags;
	}
}

/* Ensure that page table is set up for page map entries. */
static void check_space(size_t pfn)
{
	size_t req_len, off, tbl_off;
	const unsigned int flags = PAGE_RW | PAGE_PRESENT;

	req_len = (pfn + 1) * sizeof (struct page);

	/* Check if a new virtual page needs to be mapped */
	if (req_len > npages * PAGE_SIZE) {
		/* Check if a new page table is required */
		tbl_off = ntables * PAGE_SIZE * PTRS_PER_PGTBL;
		if (req_len > tbl_off) {
			memset((void *)curr_pgtbl, 0, PGTBL_SIZE);
			__create_pgtbl(PAGE_MAP_BASE + tbl_off,
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

static void split_block(size_t pfn, size_t lim);

#define M_TO_PAGES(m) (_M(m) / PAGE_SIZE)

/* Initialize all buddy allocator lists. */
static void buddy_populate(void)
{
	size_t pfn, end, ord, section_end;

	pfn = 0;
	/* first 4 MiB are reserved for kernel usage */
	section_end = M_TO_PAGES(4);
	while (pfn < section_end) {
		ord = PM_PAGE_BLOCK_ORDER(page_map + pfn);
		end = pfn + TWO(ord);

		if (end > section_end) {
			split_block(pfn, section_end);
			end = section_end;
		}
		for (; pfn < end; ++pfn)
			page_map[pfn].status |= PM_PAGE_MAPPED |
						PM_PAGE_RESERVED;
	}

	/* addresses < 16 MiB are part of the DMA zone */
	section_end = M_TO_PAGES(16);
	while (pfn < section_end) {
		ord = PM_PAGE_BLOCK_ORDER(page_map + pfn);
		end = pfn + TWO(ord);

		if (end > section_end) {
			split_block(pfn, section_end);
			end = section_end;
			ord = PM_PAGE_BLOCK_ORDER(page_map + pfn);
		}
		if (!(page_map[pfn].status & PM_PAGE_INVALID)) {
			list_add(&zone_dma.ord[ord], &page_map[pfn].list);
			zone_dma.len[ord]++;
			for (; pfn < end; ++pfn)
				PM_SET_MAX_ORDER(page_map + pfn, ord);
		}
		pfn = end;
	}

	/* mark the pages in the page_map as reserved */
	section_end = pfn + npages;
	while (pfn < section_end) {
		ord = PM_PAGE_BLOCK_ORDER(page_map + pfn);
		end = pfn + TWO(ord);

		if (end > section_end) {
			split_block(pfn, section_end);
			end = section_end;
		}
		for (; pfn < end; ++pfn)
			page_map[pfn].status |= PM_PAGE_MAPPED |
						PM_PAGE_RESERVED;
	}

	/* finally, add all remaining valid memory to buddy lists */
	section_end = totalmem / PAGE_SIZE;
	while (pfn < section_end) {
		ord = PM_PAGE_BLOCK_ORDER(page_map + pfn);
		end = pfn + TWO(ord);

		if (!(page_map[pfn].status & PM_PAGE_INVALID)) {
			list_add(&zone_reg.ord[ord], &page_map[pfn].list);
			zone_reg.len[ord]++;
			for (; pfn < end; ++pfn)
				PM_SET_MAX_ORDER(page_map + pfn, ord);
		}
		pfn = end;
	}
}

/* Split block of pages starting at pfn into two blocks around PFN lim. */
static void split_block(size_t pfn, size_t lim)
{
	size_t ord, rem, end;

	ord = PM_PAGE_BLOCK_ORDER(page_map + pfn);
	rem = pfn + TWO(ord) - lim;
	end = lim;

	while (rem) {
		ord = order(rem);
		if (rem < TWO(ord))
			--ord;

		PM_SET_BLOCK_ORDER(page_map + end, ord);
		end += TWO(ord);
		rem -= TWO(ord);
	}

	rem = lim - pfn;
	while (rem) {
		ord = order(rem);
		if (rem < TWO(ord))
			--ord;

		PM_SET_BLOCK_ORDER(page_map + pfn, ord);
		pfn += TWO(ord);
		rem -= TWO(ord);
	}
}
