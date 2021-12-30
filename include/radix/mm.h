/*
 * include/radix/mm.h
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

#ifndef RADIX_MM_H
#define RADIX_MM_H

#include <radix/asm/mm_limits.h>
#include <radix/asm/page.h>
#include <radix/error.h>
#include <radix/mm_types.h>
#include <radix/multiboot.h>
#include <radix/types.h>

#include <stdbool.h>

// Base virtual address at which kernel code is loaded.
#define KERNEL_VIRTUAL_BASE __ARCH_KERNEL_VIRT_BASE
#define KERNEL_SIZE         0x00400000

// Base virtual address for the kernel's dynamic address space.
#define RESERVED_VIRT_BASE __ARCH_RESERVED_VIRT_BASE
#define RESERVED_SIZE      (PAGING_BASE - RESERVED_VIRT_BASE)

// Virtual address range for user processes.
#define USER_VIRTUAL_BASE __ARCH_USER_VIRT_BASE
#define USER_VIRTUAL_SIZE __ARCH_USER_VIRT_SIZE
#define USER_STACK_TOP    __ARCH_USER_STACK_TOP

#define MEM_LIMIT __ARCH_MEM_LIMIT

/*
 * Recursive mapping is used for paging structures, so they occupy the top part
 * of the virtual address space.
 */
#define PAGING_BASE  __ARCH_PAGING_BASE
#define PAGING_VADDR __ARCH_PAGING_VADDR

/* Page map starts at 16 MiB in physical memory, directly after the DMA zone. */
#define __PAGE_MAP_PHYS_BASE 0x01000000
#define PAGE_MAP_BASE        phys_to_virt(__PAGE_MAP_PHYS_BASE)

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
#define __PA_ZONE_REG (1 << 1) /* allocate from kernel zone */
#define __PA_ZONE_DMA (1 << 2) /* allocate from DMA zone */
#define __PA_ZONE_USR (1 << 3) /* allocate from user zone */
#define __PA_ZONE_LOW (1 << 4) /* allocate from lowmem */
#define __PA_NO_MAP   (1 << 5) /* don't map pages to virtual address */
#define __PA_ZERO     (1 << 6) /* zero pages when allocated */
#define __PA_READONLY (1 << 7) /* mark pages as readonly */

/* Page allocation flags */
#define PA_STANDARD  (__PA_ZONE_REG)
#define PA_READONLY  (__PA_ZONE_REG | __PA_READONLY)
#define PA_DMA       (__PA_ZONE_DMA | __PA_NO_MAP)
#define PA_USER      (__PA_ZONE_USR | __PA_NO_MAP)
#define PA_PAGETABLE (__PA_ZONE_USR | __PA_NO_MAP)
#define PA_LOWMEM    (__PA_ZONE_LOW)

struct page *alloc_pages(unsigned int flags, size_t ord);
void free_pages(struct page *p);

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
static __always_inline paddr_t page_to_phys(struct page *p)
{
    return (paddr_t)page_to_pfn(p) << PAGE_SHIFT;
}

static __always_inline struct page *phys_to_page(paddr_t phys)
{
    return page_map + (phys >> PAGE_SHIFT);
}

/*
 * Memory management functions.
 * Each supported architecture must provide its own implementation
 * in arch/$ARCH/include/radix/page.h.
 */

#define set_pde(virt, pde) __arch_set_pde(virt, pde)
#define addr_mapped(virt)  __arch_addr_mapped(virt)

#define PROT_READ  (1 << 0)
#define PROT_WRITE (1 << 1)
#define PROT_EXEC  (1 << 2)

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

struct vmm_space;

// TODO(frolv): Rewrite these to use fewer macros.

#define map_page_kernel(virt, phys, prot, cp) \
    __arch_map_page_kernel(virt, phys, prot, cp)
#define map_page_user(virt, phys, prot, cp) \
    __arch_map_page_user(virt, phys, prot, cp)

// Maps virtual to physical addresses within the kernel address space.
static inline int map_pages_kernel(
    addr_t virt, paddr_t phys, size_t num_pages, int prot, enum cache_policy cp)
{
    return __arch_map_pages(virt, phys, num_pages, prot, cp, /*user=*/false);
}

// Maps virtual to physical addresses within the address space of the current
// process.
static inline int map_pages_user(
    addr_t virt, paddr_t phys, size_t num_pages, int prot, enum cache_policy cp)
{
    return __arch_map_pages(virt, phys, num_pages, prot, cp, /*user=*/true);
}

// Maps virtual to physical addresses within a given address space.
static inline int map_pages_vmm(const struct vmm_space *vmm,
                                addr_t virt,
                                paddr_t phys,
                                size_t num_pages,
                                int prot,
                                enum cache_policy cp)
{
    return __arch_map_pages_vmm(vmm, virt, phys, num_pages, prot, cp);
}

#define unmap_pages(virt, n) __arch_unmap_pages(virt, n)
#define unmap_page(virt)     __arch_unmap_pages(virt, 1)

#define set_cache_policy(virt, type) __arch_set_cache_policy(virt, type)

#define mark_page_wb(virt)      set_cache_policy(virt, PAGE_CP_WRITE_BACK)
#define mark_page_wt(virt)      set_cache_policy(virt, PAGE_CP_WRITE_THROUGH)
#define mark_page_ucminus(virt) set_cache_policy(virt, PAGE_CP_UNCACHED)
#define mark_page_uc(virt)      set_cache_policy(virt, PAGE_CP_UNCACHEABLE)
#define mark_page_wc(virt)      set_cache_policy(virt, PAGE_CP_WRITE_COMBINING)
#define mark_page_wp(virt)      set_cache_policy(virt, PAGE_CP_WRITE_PROTECTED)

#define switch_address_space(vmm) __arch_switch_address_space(vmm)

/*
 * TLB control functions.
 */

#define tlb_flush_all(sync)           __arch_tlb_flush_all(sync)
#define tlb_flush_nonglobal(sync)     __arch_tlb_flush_nonglobal(sync)
#define tlb_flush_range(lo, hi, sync) __arch_tlb_flush_range(lo, hi, sync)
#define tlb_flush_page(addr, sync)    __arch_tlb_flush_page(addr, sync)

#define tlb_flush_nonglobal_lazy()   __arch_tlb_flush_nonglobal_lazy()
#define tlb_flush_range_lazy(lo, hi) __arch_tlb_flush_range_lazy(lo, hi)
#define tlb_flush_page_lazy(addr)    __arch_tlb_flush_page_lazy(addr)

/*
 * Cache control functions.
 */
#define cache_flush_all()      __arch_cache_flush_all()
#define cache_flush_page(addr) __arch_cache_flush_page(addr)

#endif  // RADIX_MM_H
