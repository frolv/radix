/*
 * include/radix/mm.h
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

#ifndef RADIX_MM_H
#define RADIX_MM_H

#include <radix/error.h>
#include <radix/mm_types.h>
#include <radix/multiboot.h>
#include <radix/types.h>

#include <radix/asm/mm_limits.h>
#include <radix/asm/page.h>

#define KERNEL_VIRTUAL_BASE     __ARCH_KERNEL_VIRT_BASE
#define KERNEL_SIZE             0x00400000
#define RESERVED_VIRT_BASE      __ARCH_RESERVED_VIRT_BASE
#define RESERVED_SIZE           (PGDIR_BASE - RESERVED_VIRT_BASE)
#define MEM_LIMIT               __ARCH_MEM_LIMIT

/*
 * The final entry in the page directory is mapped to the page directory itself.
 */
#define PGDIR_BASE              __ARCH_PGDIR_BASE
#define PGDIR_VADDR             __ARCH_PGDIR_VADDR

/* Page map starts at 16 MiB in physical memory, directly after the DMA zone. */
#define __PAGE_MAP_PHYS_BASE    0x01000000
#define PAGE_MAP_BASE           phys_to_virt(__PAGE_MAP_PHYS_BASE)


uint64_t totalmem(void);
uint64_t usedmem(void);

void buddy_init(struct multiboot_info *mbt);


/*
 * The maximum amount of pages that can be allocated
 * at a time is 2^{PA_MAX_ORDER}.
 */
#define PA_ORDERS    10U
#define PA_MAX_ORDER (PA_ORDERS - 1U)

/* Low level page allocation flags */
#define __PA_ZONE_REG   (1 << 1)        /* allocate from kernel zone */
#define __PA_ZONE_DMA   (1 << 2)        /* allocate from DMA zone */
#define __PA_ZONE_USR   (1 << 3)        /* allocate from user zone */
#define __PA_NO_MAP     (1 << 4)        /* don't map pages to virtual address */
#define __PA_ZERO       (1 << 5)        /* zero pages when allocated */
#define __PA_READONLY   (1 << 6)        /* mark pages as readonly */

/* Page allocation flags */
#define PA_STANDARD     (__PA_ZONE_REG)
#define PA_READONLY     (__PA_ZONE_REG | __PA_READONLY)
#define PA_DMA          (__PA_ZONE_DMA | __PA_NO_MAP)
#define PA_USER         (__PA_ZONE_USR | __PA_NO_MAP)
#define PA_PAGETABLE    (__PA_ZONE_REG | __PA_NO_MAP)

struct page *alloc_pages(unsigned int flags, size_t ord);
void free_pages(struct page *p);
void mark_page_mapped(struct page *p, addr_t virt);

static __always_inline struct page *alloc_page(unsigned int flags)
{
	return alloc_pages(flags, 0);
}

#define virt_to_phys(x) __arch_pa((addr_t)(x))
#define phys_to_virt(x) __arch_va((addr_t)(x))

extern struct page *page_map;

#define PFN(x) (virt_to_phys(x) >> PAGE_SHIFT)

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

static __always_inline struct page *phys_to_page(addr_t phys)
{
	return page_map + (phys >> PAGE_SHIFT);
}

/*
 * Memory management functions.
 * Each supported architecture must provide its own implementation
 * in arch/$ARCH/include/radix/page.h.
 */

#define set_pde(virt, pde)              __arch_set_pde(virt, pde)
#define addr_mapped(virt)               __arch_addr_mapped(virt)

#define PROT_READ  0
#define PROT_WRITE 1

/* CPU caching control */
enum cache_policy {
	PAGE_CP_DEFAULT,
	PAGE_CP_WRITE_BACK,
	PAGE_CP_WRITE_THROUGH,
	PAGE_CP_UNCACHED,
	PAGE_CP_UNCACHEABLE,
	PAGE_CP_WRITE_COMBINING,
	PAGE_CP_WRITE_PROTECTED
};

#define map_page_kernel(virt, phys, prot, cp) \
	__arch_map_page_kernel(virt, phys, prot, cp)
#define map_page_user(virt, phys, prot, cp) \
	__arch_map_page_user(virt, phys, prot, cp)

#define map_pages_kernel(virt, phys, prot, cp, n) \
	__arch_map_pages(virt, phys, prot, cp, 0, n)
#define map_pages_user(virt, phys, prot, cp, n) \
	__arch_map_pages(virt, phys, prot, cp, 1, n)

#define unmap_page(virt)                __arch_unmap_page(virt)
#define unmap_page_clean(virt)          __arch_unmap_page_clean(virt)
#define unmap_pages(virt, n)            __arch_unmap_pages(virt, n)

#define set_cache_policy(virt, type)    __arch_set_cache_policy(virt, type)

#define mark_page_wb(virt)      set_cache_policy(virt, PAGE_CP_WRITE_BACK)
#define mark_page_wt(virt)      set_cache_policy(virt, PAGE_CP_WRITE_THROUGH)
#define mark_page_ucminus(virt) set_cache_policy(virt, PAGE_CP_UNCACHED)
#define mark_page_uc(virt)      set_cache_policy(virt, PAGE_CP_UNCACHEABLE)
#define mark_page_wc(virt)      set_cache_policy(virt, PAGE_CP_WRITE_COMBINING)
#define mark_page_wp(virt)      set_cache_policy(virt, PAGE_CP_WRITE_PROTECTED)

/*
 * TLB control functions.
 */

#define tlb_flush_all(sync)             __arch_tlb_flush_all(sync)
#define tlb_flush_nonglobal(sync)       __arch_tlb_flush_nonglobal(sync)
#define tlb_flush_range(lo, hi, sync)   __arch_tlb_flush_range(lo, hi, sync)
#define tlb_flush_page(addr, sync)      __arch_tlb_flush_page(addr, sync)

/*
 * The lazy versions of the tlb_* functions use the approach of lazy TLB
 * invalidation. TLB entries are only invalidated immediately on the running
 * processor; if other processors attempt to access an invalid page in the
 * future, they will page fault. The page fault handler will then detect that
 * the fault occurred because of an invalid TLB entry, and remove it.
 * This has the advantage of avoiding the overhead of multiple inter-processor
 * interrupts when an entry is invalidated.
 * These functions should be preferred over their non-lazy counterparts,
 * when possible.
 *
 * `tlb_flush_all` does not have a lazy version. In the case that a call to
 * it is warranted, it is probably essential that all processors' TLBs get
 * flushed immediately.
 */
#define tlb_flush_nonglobal_lazy()      __arch_tlb_flush_nonglobal_lazy()
#define tlb_flush_range_lazy(lo, hi)    __arch_tlb_flush_range_lazy(lo, hi)
#define tlb_flush_page_lazy(addr)       __arch_tlb_flush_page_lazy(addr)

/*
 * Cache control functions.
 */
#define cache_flush_all()               __arch_cache_flush_all()
#define cache_flush_page(addr)          __arch_cache_flush_page(addr)

#endif /* RADIX_MM_H */
