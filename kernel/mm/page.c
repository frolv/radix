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

#include <stdio.h>
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
/* Memory for kernel use. */
static struct buddy zone_reg;
/* The rest of memory. */
static struct buddy zone_usr;

/* total amount of usable memory in the system */
uint64_t totalmem = 0;

uint64_t zone_reg_end = 0;

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
		if (base > MEM_LIMIT - PAGE_SIZE)
			base = MEM_LIMIT - PAGE_SIZE;

		/*
		 * next_phys_region only returns valid memory regions,
		 * but all existing memory needs to be mapped.
		 */
		if (base != next)
			init_region(next, base - next, PM_PAGE_INVALID);

		if (base + len >= MEM_LIMIT - PAGE_SIZE) {
			printf("\nThis system appears to have more than the "
					"supported limit\nof %llu MiB of "
					"memory. Only the first %llu MiB will "
					"be used.\n\n", MEM_LIMIT / _M(1),
					MEM_LIMIT / _M(1));
			break;
		}
		init_region(base, len, 0);
		next = base + len;
	}

	/*
	 * The regular zone is the memory that is set aside for kernel usage.
	 * It extends from the end of the DMA zone up to 1/4 of total memory,
	 * or a maximum of 1 GiB.
	 */
	zone_reg_end = totalmem / 4;
	if (zone_reg_end < _M(20)) {
		if (totalmem > _M(16))
			zone_reg_end = _M(20);
		else
			zone_reg_end = 0;
	}
	else if (zone_reg_end > _G(1))
		zone_reg_end = _G(1);

	/* initialize buddy zones */
	for (i = 0; i < PA_MAX_ORDER; ++i) {
		list_init(&zone_dma.ord[i]);
		zone_dma.len[i] = 0;
		list_init(&zone_reg.ord[i]);
		zone_reg.len[i] = 0;
		list_init(&zone_usr.ord[i]);
		zone_usr.len[i] = 0;
	}
	zone_dma.max_ord = 0;
	zone_reg.max_ord = 0;
	zone_usr.max_ord = 0;

	buddy_populate();
}

static struct page *__alloc_pages(struct buddy *zone,
				  unsigned int flags, size_t ord);
static void buddy_split(struct buddy *zone, size_t req_ord);
static struct page *buddy_coalesce(struct buddy *zone, struct page *p);

/*
 * Allocate a contiguous block of pages in memory.
 * Behaviour of the allocator is managed by flags.
 */
struct page *alloc_pages(unsigned int flags, size_t ord)
{
	struct buddy *zone;

	if (ord > PA_MAX_ORDER - 1)
		return ERR_PTR(EINVAL);

	/* TODO: if zone is full, allocate from another */
	if (flags & __PA_ZONE_DMA)
		zone = &zone_dma;
	else if (flags & __PA_ZONE_USR)
		zone = &zone_usr;
	else
		zone = &zone_reg;

	return __alloc_pages(zone, flags, ord);
}

/* Free the block of pages starting at p. */
void free_pages(struct page *p)
{
	struct buddy *zone;
	size_t ord;

	/* make sure p is the start of an allocated block */
	if (!(p->status & PM_PAGE_ALLOCATED))
		return;
	if (PM_PAGE_BLOCK_ORDER(p) == PM_PAGE_ORDER_INNER)
		return;

	if (page_to_phys(p) < _M(16))
		zone = &zone_dma;
	else
		zone = (p->status & PM_PAGE_ZONE_USR) ? &zone_usr : &zone_reg;

	p->slab_cache = (void *)PAGE_UNINIT_MAGIC;
	p->slab_desc = (void *)PAGE_UNINIT_MAGIC;
	p->status &= ~PM_PAGE_ALLOCATED;

	if (PM_PAGE_BLOCK_ORDER(p) < PM_PAGE_MAX_ORDER(p))
		p = buddy_coalesce(zone, p);
	ord = PM_PAGE_BLOCK_ORDER(p);

	list_add(&zone->ord[ord], &p->list);
	zone->len[ord]++;
	zone->max_ord = MAX(zone->max_ord, ord);
}

/* Allocate 2^{ord} pages from zone. */
static struct page *__alloc_pages(struct buddy *zone,
				  unsigned int flags, size_t ord)
{
	struct page *p;
	addr_t virt;

	if (unlikely(ord > zone->max_ord))
		return ERR_PTR(ENOMEM);

	/* split larger blocks until one of the requested order exists */
	if (!zone->len[ord])
		buddy_split(zone, ord);

	p = list_first_entry(&zone->ord[ord], struct page, list);
	list_del(&p->list);
	zone->len[ord]--;
	if (ord == zone->max_ord) {
		while (!zone->len[zone->max_ord])
			zone->max_ord--;
	}

	if (!(flags & __PA_NO_MAP) && !(p->status & PM_PAGE_MAPPED)) {
		if (zone == &zone_reg) {
			virt = page_to_phys(p) + KERNEL_VIRTUAL_BASE;
		} else {
			/* TODO: find free virtual address range, map pages */
			virt = 0;
		}

		/* map_pages(page_to_phys(p), virt, POW2(ord)); */
		p->mem = (void *)virt;
		p->status |= PM_PAGE_MAPPED;
	}

	p->status |= PM_PAGE_ALLOCATED;
	return p;
}

/* Split a block of pages in zone to get a block of size req_ord. */
static void buddy_split(struct buddy *zone, size_t req_ord)
{
	size_t ord;
	struct page *p, *buddy;

	/* find the first available block order greater than req_ord */
	for (ord = req_ord; ord <= zone->max_ord; ++ord) {
		if (zone->len[ord])
			break;
	}

	p = list_first_entry(&zone->ord[ord], struct page, list);

	while (!zone->len[req_ord]) {
		buddy = p + POW2(ord - 1);

		list_del(&p->list);
		zone->len[ord]--;
		--ord;

		PM_SET_BLOCK_ORDER(p, ord);
		PM_SET_BLOCK_ORDER(buddy, ord);
		list_add(&zone->ord[ord], &buddy->list);
		list_add(&zone->ord[ord], &p->list);
		zone->len[ord] += 2;
	}
}

/*
 * Merge page block starting at p with its buddies as far as possible.
 * Return pointer to the start of new merged page block.
 */
static struct page *buddy_coalesce(struct buddy *zone, struct page *p)
{
	size_t ord, block_off;
	struct page *buddy;

	do {
		block_off = PM_PAGE_BLOCK_OFFSET(p);
		ord = PM_PAGE_BLOCK_ORDER(p);
		if (ALIGNED(block_off, POW2(ord + 1)))
			buddy = p + POW2(ord);
		else
			buddy = p - POW2(ord);

		/*
		 * Two blocks can be coalesced if they are both
		 * of the same order and both unallocated.
		 */
		if (PM_PAGE_BLOCK_ORDER(buddy) != PM_PAGE_BLOCK_ORDER(p))
			return p;
		if (buddy->status & PM_PAGE_ALLOCATED)
			return p;

		list_del(&buddy->list);
		zone->len[ord]--;

		/* set p to point to the base of the new, larger block */
		if (p > buddy)
			SWAP(p, buddy);

		PM_SET_BLOCK_ORDER(buddy, PM_PAGE_ORDER_INNER);
		PM_SET_BLOCK_ORDER(p, ord + 1);
	} while (ord < PM_PAGE_MAX_ORDER(p));

	return p;
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
		if (pages < POW2(ord))
			--ord;

		end = base + POW2(ord) * PAGE_SIZE;

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

static size_t zone_init(size_t pfn, size_t section_end,
			struct buddy *zone, unsigned int flags);

#define M_TO_PAGES(m) (_M(m) / PAGE_SIZE)

/* Initialize all buddy allocator lists. */
static void buddy_populate(void)
{
	size_t pfn;
	const unsigned int kflags = PM_PAGE_MAPPED | PM_PAGE_RESERVED;

	/* first 4 MiB are reserved for kernel usage */
	pfn = zone_init(0, M_TO_PAGES(4), NULL, kflags);
	/* addresses < 16 MiB are part of the DMA zone */
	pfn = zone_init(pfn, M_TO_PAGES(16), &zone_dma, 0);
	/* mark the pages in the page_map as reserved */
	pfn = zone_init(pfn, pfn + npages, NULL, kflags);
	pfn = zone_init(pfn, zone_reg_end / PAGE_SIZE, &zone_reg, 0);
	pfn = zone_init(pfn, totalmem / PAGE_SIZE, &zone_usr, PM_PAGE_ZONE_USR);
}

/* Split block of pages starting at pfn into two blocks around PFN lim. */
static void split_block(size_t pfn, size_t lim)
{
	size_t ord, rem, end;

	ord = PM_PAGE_BLOCK_ORDER(page_map + pfn);
	rem = pfn + POW2(ord) - lim;
	end = lim;

	while (rem) {
		ord = order(rem);
		if (rem < POW2(ord))
			--ord;

		PM_SET_BLOCK_ORDER(page_map + end, ord);
		end += POW2(ord);
		rem -= POW2(ord);
	}

	rem = lim - pfn;
	while (rem) {
		ord = order(rem);
		if (rem < POW2(ord))
			--ord;

		PM_SET_BLOCK_ORDER(page_map + pfn, ord);
		pfn += POW2(ord);
		rem -= POW2(ord);
	}
}

static size_t zone_init(size_t pfn, size_t section_end,
			struct buddy *zone, unsigned int flags)
{
	size_t ord, end, start;

	while (pfn < section_end) {
		ord = PM_PAGE_BLOCK_ORDER(page_map + pfn);
		end = pfn + POW2(ord);

		if (end > section_end) {
			split_block(pfn, section_end);
			ord = PM_PAGE_BLOCK_ORDER(page_map + pfn);
			end = pfn + POW2(ord);
		}

		if (zone && !(page_map[pfn].status & PM_PAGE_INVALID)) {
			list_add(&zone->ord[ord], &page_map[pfn].list);
			zone->len[ord]++;
			zone->max_ord = MAX(ord, zone->max_ord);
		}
		start = pfn;
		for (; pfn < end; ++pfn) {
			page_map[pfn].status |= flags;
			PM_SET_MAX_ORDER(page_map + pfn, ord);
			PM_SET_PAGE_OFFSET(page_map + pfn, pfn - start);
		}
		pfn = end;
	}

	return pfn;
}
