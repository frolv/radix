/*
 * kernel/mm/page.c
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

#include <radix/bits.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/mm.h>
#include <radix/vmm.h>

#include <rlibc/string.h>

#include "buddy.h"

struct page *page_map = (struct page *)PAGE_MAP_BASE;
addr_t page_map_end = PAGE_MAP_BASE;

/* First 1 MiB of physical memory. */
static struct buddy zone_low = BUDDY_INIT;
/* Physical memory under 16 MiB. */
static struct buddy zone_dma = BUDDY_INIT;
/* Memory for kernel use. */
static struct buddy zone_reg = BUDDY_INIT;
/* The rest of memory. */
static struct buddy zone_usr = BUDDY_INIT;

#define __PA_UNMAPPABLE (1 << 31)

/* total amount of usable memory in the system */
static uint64_t memsize = 0;
static uint64_t memused = 0;

static uint64_t phys_mem_end = 0;
static uint64_t zone_reg_end = 0;

static int next_phys_region(struct multiboot_info *mbt,
                            uint64_t *base, uint64_t *len);
static void init_region(uint64_t base, uint64_t len, unsigned int flags);
static void buddy_populate(void);

uint64_t totalmem(void)
{
	return memsize;
}

uint64_t usedmem(void)
{
	return memused;
}

void buddy_init(struct multiboot_info *mbt)
{
	uint64_t base, len, next;
	size_t i;

	/* mmap_addr stores the physical address of the memory map */
	mbt->mmap_addr = phys_to_virt(mbt->mmap_addr);

	next = 0;
	while (next_phys_region(mbt, &base, &len) == 0) {
		if (base > MEM_LIMIT - PAGE_SIZE)
			base = MEM_LIMIT - PAGE_SIZE;

		/*
		 * next_phys_region only returns valid memory regions,
		 * but all existing memory needs to be mapped.
		 */
		if (base != next)
			init_region(next, base - next, PM_PAGE_INVALID);

		if (base + len > MEM_LIMIT - PAGE_SIZE) {
			len = (base + len) - (MEM_LIMIT - PAGE_SIZE);
			init_region(base, len, 0);
			break;
		}

		init_region(base, len, 0);
		next = base + len;
	}
	phys_mem_end = next;

	/*
	 * The regular zone is the memory that is set aside for kernel usage.
	 * It extends from the end of the DMA zone up to 1/8 of total memory,
	 * or a maximum of 256 MiB.
	 */
	zone_reg_end = memsize / 8;
	if (zone_reg_end < MIB(20))
		zone_reg_end = memsize > MIB(16) ? MIB(20) : 0;
	else if (zone_reg_end > RESERVED_VIRT_BASE - KERNEL_VIRTUAL_BASE)
		zone_reg_end = RESERVED_VIRT_BASE - KERNEL_VIRTUAL_BASE;

	/* initialize buddy zone lists */
	for (i = 0; i < PA_ORDERS; ++i) {
		list_init(&zone_low.ord[i]);
		list_init(&zone_dma.ord[i]);
		list_init(&zone_reg.ord[i]);
		list_init(&zone_usr.ord[i]);
	}

	buddy_populate();
}

static struct page *__alloc_pages(struct buddy *zone,
                                  unsigned int flags, size_t ord);
static void buddy_split(struct buddy *zone, size_t req_ord);
static struct page *buddy_coalesce(struct buddy *zone, struct page *p);

/*
 * alloc_pages:
 * Allocate a contiguous block of pages in memory.
 * Behaviour of the allocator is managed by flags.
 */
struct page *alloc_pages(unsigned int flags, size_t ord)
{
	struct buddy *zone;
	struct page *ret;

	if (ord > PA_MAX_ORDER)
		return ERR_PTR(EINVAL);

	if (flags & __PA_ZONE_DMA) {
		zone = &zone_dma;
		flags |= __PA_UNMAPPABLE;
	} else if (flags & __PA_ZONE_USR) {
		zone = &zone_usr;
		flags |= __PA_UNMAPPABLE;
	} else if (flags & __PA_ZONE_LOW) {
		zone = &zone_low;
	} else {
		zone = &zone_reg;
	}

	if ((flags & __PA_UNMAPPABLE) && !(flags & __PA_NO_MAP))
		return ERR_PTR(EINVAL);

	spin_lock(&zone->lock);

	/* TODO: if zone is full, allocate from another */
	if (ord > zone->max_ord || zone->alloc_pages == zone->total_pages)
		ret = ERR_PTR(ENOMEM);
	else
		ret = __alloc_pages(zone, flags, ord);

	spin_unlock(&zone->lock);

	return ret;
}

/* free_pages: free the block of pages starting at `p` */
void free_pages(struct page *p)
{
	struct buddy *zone;
	size_t ord;

	/* make sure p is the start of an allocated block */
	if (!(p->status & PM_PAGE_ALLOCATED))
		return;
	if (PM_PAGE_BLOCK_ORDER(p) == PM_PAGE_ORDER_INNER)
		return;


	p->slab_cache = (void *)PAGE_UNINIT_MAGIC;
	p->slab_desc = (void *)PAGE_UNINIT_MAGIC;
	p->status &= ~PM_PAGE_ALLOCATED;
	ord = PM_PAGE_BLOCK_ORDER(p);

	if (page_to_phys(p) < MIB(1)) {
		zone = &zone_low;
	} else if (page_to_phys(p) < MIB(16)) {
		zone = &zone_dma;
	} else if (p->status & PM_PAGE_ZONE_USR) {
		zone = &zone_usr;
		if (p->status & PM_PAGE_MAPPED) {
			unmap_pages((addr_t)p->mem, ord);
			p->mem = (void *)PAGE_UNINIT_MAGIC;
			p->status &= ~PM_PAGE_MAPPED;
		}
	} else {
		zone = &zone_reg;
	}

	spin_lock(&zone->lock);

	zone->alloc_pages -= pow2(ord);
	memused -= pow2(ord) * PAGE_SIZE;

	if (ord < PM_PAGE_MAX_ORDER(p)) {
		p = buddy_coalesce(zone, p);
		ord = PM_PAGE_BLOCK_ORDER(p);
	}

	list_add(&zone->ord[ord], &p->list);
	zone->len[ord]++;
	zone->max_ord = max(zone->max_ord, ord);

	spin_unlock(&zone->lock);
}

/* __alloc_pages: allocate 2^{ord} pages from `zone` */
static struct page *__alloc_pages(struct buddy *zone,
                                  unsigned int flags, size_t ord)
{
	struct page *p;
	addr_t virt;
	int npages, prot;

	/* split larger blocks until one of the requested order exists */
	if (!zone->len[ord])
		buddy_split(zone, ord);

	p = list_first_entry(&zone->ord[ord], struct page, list);
	list_del(&p->list);
	zone->len[ord]--;
	if (zone->max_ord && ord == zone->max_ord) {
		while (!zone->len[zone->max_ord])
			zone->max_ord--;
	}

	npages = pow2(ord);
	zone->alloc_pages += npages;
	memused += npages * PAGE_SIZE;

	if (!(flags & __PA_NO_MAP) && !(p->status & PM_PAGE_MAPPED)) {
		if (zone == &zone_reg)
			virt = phys_to_virt(page_to_phys(p));
		else
			virt = (addr_t)vmalloc(npages * PAGE_SIZE);

		prot = flags & __PA_READONLY ? PROT_READ : PROT_WRITE;
		map_pages_kernel(virt, page_to_phys(p), prot,
		                 PAGE_CP_DEFAULT, npages);

		if (flags & __PA_ZERO)
			memset((void *)virt, 0, npages * PAGE_SIZE);

		p->mem = (void *)virt;
		p->status |= PM_PAGE_MAPPED;
	}

	p->status |= PM_PAGE_ALLOCATED;
	return p;
}

/*
 * buddy_split:
 * Split a block of pages in `zone` to get a block of size `req_ord`.
 */
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
		buddy = p + pow2(ord - 1);

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
 * buddy_coalesce:
 * Merge page block starting at `p` with its buddies as far as possible.
 * Return pointer to the start of new merged page block.
 */
static struct page *buddy_coalesce(struct buddy *zone, struct page *p)
{
	size_t ord, block_off;
	struct page *buddy;

	do {
		block_off = PM_PAGE_BLOCK_OFFSET(p);
		ord = PM_PAGE_BLOCK_ORDER(p);
		if (ALIGNED(block_off, pow2(ord + 1)))
			buddy = p + pow2(ord);
		else
			buddy = p - pow2(ord);

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
			swap(p, buddy);

		PM_SET_BLOCK_ORDER(buddy, PM_PAGE_ORDER_INNER);
		PM_SET_BLOCK_ORDER(p, ord + 1);
	} while (ord < PM_PAGE_MAX_ORDER(p));

	return p;
}

/*
 * mark_page_mapped:
 * Indicate that page `p` has been mapped to address `virt`.
 */
void mark_page_mapped(struct page *p, addr_t virt)
{
	p->mem = (void *)virt;
	p->status |= PM_PAGE_MAPPED;
	PM_SET_REFCOUNT(p, 1);
}

static struct memory_map *mmap = NULL;

#define NEXT_MAP(mmap) \
	((struct memory_map *)((addr_t)mmap + mmap->size + sizeof (mmap->size)))

#define IN_RANGE(mmap, mbt) \
	((addr_t)mmap < mbt->mmap_addr + mbt->mmap_length)

#define make64(low, high) ((uint64_t)(high) << 32 | (low))

static void klog_mmap(struct memory_map *mmap)
{
	uint64_t base, len;

	if (!mmap->type)
		return;

	base = make64(mmap->base_addr_low, mmap->base_addr_high);
	len = make64(mmap->length_low, mmap->length_high);

	klog(KLOG_INFO, "physmem: 0x%012llX-0x%012llX %s",
	     base, base + len,
	     mmap->type == 1 ? "available" : "reserved");
}

/*
 * next_phys_region:
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

	klog_mmap(mmap);

	/* only consider available RAM */
	while (mmap->type != 1 && IN_RANGE(mmap, mbt)) {
		memsize += make64(mmap->length_low, mmap->length_high);
		mmap = NEXT_MAP(mmap);
		klog_mmap(mmap);
	}

	if (!IN_RANGE(mmap, mbt))
		return 1;

	b = make64(mmap->base_addr_low, mmap->base_addr_high);
	l = make64(mmap->length_low, mmap->length_high);

	/*
	 * This should already be aligned by the bootloader.
	 * But, just in case...
	 */
	orig_base = b;
	b = ALIGN(b, PAGE_SIZE);
	l = (l - (b - orig_base)) & ~((uint64_t)PAGE_SIZE - 1);

	*base = b;
	*len = l;
	memsize += l;

	return 0;
}

/*
 * Allocate required page tables for the page map
 * going backwards from the end of the kernel.
 */
static addr_t curr_pgtbl = KERNEL_VIRTUAL_BASE + KERNEL_SIZE - PGTBL_SIZE;
static size_t ntables = 0;

/* Number of pages used for the page_map */
static size_t npages = 0;

static void check_space(size_t pfn, size_t pages);

/*
 * init_region:
 * Populate struct pages for a region of physical memory starting at base.
 */
static void init_region(uint64_t base, uint64_t len, unsigned int flags)
{
	size_t ord, pfn, start, pages;
	uint64_t end;

	while (len) {
		pages = len / PAGE_SIZE;

		/* determine the size of the block, up the the maximum */
		if ((ord = log2(pages)) > PA_MAX_ORDER)
			ord = PA_MAX_ORDER;
		if (pages < pow2(ord))
			--ord;

		pages = pow2(ord);
		end = base + pages * PAGE_SIZE;

		/* initialize all pages in the block */
		start = base >> PAGE_SHIFT;
		check_space(start, pages);
		for (; base < end; base += PAGE_SIZE) {
			pfn = base >> PAGE_SHIFT;

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

/*
 * check_table_space:
 * Ensure sufficient page tables to map pages from
 * PAGE_MAP_BASE to PAGE_MAP_BASE + `req_len`.
 */
static void check_table_space(size_t req_len)
{
	size_t off;
	const unsigned int flags = PAGE_GLOBAL | PAGE_RW | PAGE_PRESENT;

	off = ntables * PAGE_SIZE * PTRS_PER_PGTBL;
	if (req_len > off) {
		for (; off < req_len; off += PAGE_SIZE * PTRS_PER_PGTBL) {
			memset((void *)curr_pgtbl, 0, PGTBL_SIZE);
			set_pde(PAGE_MAP_BASE + off,
			        make_pde(virt_to_phys(curr_pgtbl) | flags));
			curr_pgtbl -= PGTBL_SIZE;
			++ntables;
		}
	}
}

/* check_space: ensure sufficient space in page map */
static void check_space(size_t pfn, size_t pages)
{
	size_t req_len, off;

	req_len = (pfn + pages) * sizeof (struct page);
	off = npages * PAGE_SIZE;

	/* check if pages need to be mapped */
	if (req_len > off) {
		check_table_space(req_len);

		for (; off < req_len; off += PAGE_SIZE) {
			map_page_kernel(PAGE_MAP_BASE + off,
			                __PAGE_MAP_PHYS_BASE + off,
			                PROT_WRITE, PAGE_CP_DEFAULT);
			++npages;
			page_map_end += PAGE_SIZE;
		}
	}
}

static size_t zone_init(size_t pfn, size_t section_end,
                        struct buddy *zone, unsigned int flags);

#define M_TO_PAGES(m) (MIB(m) / PAGE_SIZE)

/* buddy_populate: initialize all buddy allocator lists */
static void buddy_populate(void)
{
	size_t pfn;
	const unsigned int kflags = PM_PAGE_MAPPED | PM_PAGE_RESERVED;

	pfn = zone_init(0, M_TO_PAGES(1), &zone_low, PM_PAGE_MAPPED);
	pfn = zone_init(pfn, M_TO_PAGES(4), NULL, kflags);
	/* addresses < 16 MiB are part of the DMA zone */
	pfn = zone_init(pfn, M_TO_PAGES(16), &zone_dma, 0);
	/* mark the pages in the page_map as reserved */
	pfn = zone_init(pfn, pfn + npages, NULL, kflags);
	pfn = zone_init(pfn, zone_reg_end / PAGE_SIZE, &zone_reg, 0);
	pfn = zone_init(pfn, phys_mem_end / PAGE_SIZE, &zone_usr, PM_PAGE_ZONE_USR);
}

/*
 * split_block:
 * Split block of pages starting at `pfn` into two blocks around PFN `lim`.
 */
static void split_block(size_t pfn, size_t lim)
{
	size_t ord, rem, end;

	ord = PM_PAGE_BLOCK_ORDER(page_map + pfn);
	rem = pfn + pow2(ord) - lim;
	end = lim;

	while (rem) {
		ord = log2(rem);
		if (rem < pow2(ord))
			--ord;

		PM_SET_BLOCK_ORDER(page_map + end, ord);
		end += pow2(ord);
		rem -= pow2(ord);
	}

	rem = lim - pfn;
	while (rem) {
		ord = log2(rem);
		if (rem < pow2(ord))
			--ord;

		PM_SET_BLOCK_ORDER(page_map + pfn, ord);
		pfn += pow2(ord);
		rem -= pow2(ord);
	}
}

/*
 * zone_init:
 * Add all struct pages between `pfn` and `section_end` to `zone`
 * and set specified page flags.
 */
static size_t zone_init(size_t pfn, size_t section_end,
                        struct buddy *zone, unsigned int flags)
{
	size_t ord, end, start;

	while (pfn < section_end) {
		ord = PM_PAGE_BLOCK_ORDER(page_map + pfn);
		end = pfn + pow2(ord);

		/*
		 * The current block of pages exceeds the remaining space in
		 * this zone. Split it into two parts add the first to the zone.
		 */
		if (end > section_end) {
			split_block(pfn, section_end);
			ord = PM_PAGE_BLOCK_ORDER(page_map + pfn);
			end = pfn + pow2(ord);
		}

		/* ignore invalid pages */
		if (!(page_map[pfn].status & PM_PAGE_INVALID)) {
			if (zone) {
				list_add(&zone->ord[ord], &page_map[pfn].list);
				zone->len[ord]++;
				zone->max_ord = max(ord, zone->max_ord);
				zone->total_pages += pow2(ord);
			}
			if (flags & PM_PAGE_RESERVED)
				memused += pow2(ord) * PAGE_SIZE;
		}
		start = pfn;
		for (; pfn < end; ++pfn) {
			page_map[pfn].status |= flags;
			if (flags & PM_PAGE_MAPPED)
				page_map[pfn].mem =
					(void *)phys_to_virt(pfn << PAGE_SHIFT);

			PM_SET_MAX_ORDER(page_map + pfn, ord);
			PM_SET_PAGE_OFFSET(page_map + pfn, pfn - start);
		}
		pfn = end;
	}

	return pfn;
}
