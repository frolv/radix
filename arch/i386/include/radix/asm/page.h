/*
 * arch/i386/include/radix/asm/page.h
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

#ifndef ARCH_I386_RADIX_PAGE_H
#define ARCH_I386_RADIX_PAGE_H

#if !defined(RADIX_MM_H) && !defined(RADIX_VMM_H)
#error <radix/asm/page.h> cannot be included directly
#endif

#define PTRS_PER_PGDIR          0x400
#define PTRS_PER_PGTBL          0x400
#define PGDIR_SIZE              (PTRS_PER_PGDIR * sizeof (void *))
#define PGTBL_SIZE              (PTRS_PER_PGTBL * sizeof (void *))

#define PGDIR_SHIFT             22
#define PAGE_SHIFT              12
#define PAGE_SIZE               (1UL << PAGE_SHIFT)
#define PAGE_MASK               (~(PAGE_SIZE - 1))

#define PGDIR_INDEX(x)          ((x) >> PGDIR_SHIFT)
#define PGTBL_INDEX(x)          (((x) >> PAGE_SHIFT) & 0x3FF)

#define _PAGE_BIT_PRESENT       0
#define _PAGE_BIT_RW            1
#define _PAGE_BIT_USER          2
#define _PAGE_BIT_PWT           3
#define _PAGE_BIT_PCD           4
#define _PAGE_BIT_ACCESSED      5
#define _PAGE_BIT_DIRTY         6
#define _PAGE_BIT_PAT           7
#define _PAGE_BIT_GLOBAL        8

#define PAGE_PRESENT    (((pteval_t)1) << _PAGE_BIT_PRESENT)
#define PAGE_RW         (((pteval_t)1) << _PAGE_BIT_RW)
#define PAGE_USER       (((pteval_t)1) << _PAGE_BIT_USER)
#define PAGE_PWT        (((pteval_t)1) << _PAGE_BIT_PWT)
#define PAGE_PCD        (((pteval_t)1) << _PAGE_BIT_PCD)
#define PAGE_ACCESSED   (((pteval_t)1) << _PAGE_BIT_ACCESSED)
#define PAGE_DIRTY      (((pteval_t)1) << _PAGE_BIT_DIRTY)
#define PAGE_PAT        (((pteval_t)1) << _PAGE_BIT_PAT)
#define PAGE_GLOBAL     (((pteval_t)1) << _PAGE_BIT_GLOBAL)

#include <radix/compiler.h>
#include <radix/asm/mm_types.h>
#include <radix/asm/mm_limits.h>

#define PDE(x) ((x).pde)
#define PTE(x) ((x).pte)

static __always_inline pde_t make_pde(pdeval_t val)
{
	return (pde_t){ val };
}

static __always_inline pte_t make_pte(pteval_t val)
{
	return (pte_t){ val };
}

#include <radix/types.h>

enum cache_policy;

/*
 * i386 definitions of generic memory management functions.
 */
addr_t i386_virt_to_phys(addr_t addr);
void i386_set_pde(addr_t virt, pde_t pde);
int i386_addr_mapped(addr_t virt);
int i386_map_page_kernel(addr_t virt, addr_t phys, int prot, int cp);
int i386_map_page_user(addr_t virt, addr_t phys, int prot, int cp);
int i386_map_pages(addr_t virt, addr_t phys, int prot,
                   int cp, int user, size_t n);
int i386_unmap_page(addr_t virt);
int i386_unmap_page_clean(addr_t virt);
int i386_set_cache_policy(addr_t virt, enum cache_policy policy);

void i386_tlb_flush_all(int sync);
void i386_tlb_flush_nonglobal(int sync);
void i386_tlb_flush_nonglobal_lazy(void);
void i386_tlb_flush_range(addr_t start, addr_t end, int sync);
void i386_tlb_flush_range_lazy(addr_t start, addr_t end);
void i386_tlb_flush_page(addr_t addr, int sync);
void i386_tlb_flush_page_lazy(addr_t addr);

static __always_inline addr_t __arch_pa(addr_t v)
{
	if (v < __ARCH_KERNEL_VIRT_BASE || v >= __ARCH_RESERVED_VIRT_BASE)
		return i386_virt_to_phys(v);
	else
		return v - __ARCH_KERNEL_VIRT_BASE;
}

#define __arch_va(addr) ((addr) + __ARCH_KERNEL_VIRT_BASE)

#define __arch_set_pde          i386_set_pde
#define __arch_addr_mapped      i386_addr_mapped
#define __arch_map_page_kernel  i386_map_page_kernel
#define __arch_map_page_user    i386_map_page_user
#define __arch_map_pages        i386_map_pages
#define __arch_unmap_page       i386_unmap_page
#define __arch_unmap_page_clean i386_unmap_page_clean
#define __arch_set_cache_policy i386_set_cache_policy

#define __arch_tlb_flush_all            i386_tlb_flush_all
#define __arch_tlb_flush_nonglobal      i386_tlb_flush_nonglobal
#define __arch_tlb_flush_nonglobal_lazy i386_tlb_flush_nonglobal_lazy
#define __arch_tlb_flush_range          i386_tlb_flush_range
#define __arch_tlb_flush_range_lazy     i386_tlb_flush_range_lazy
#define __arch_tlb_flush_page           i386_tlb_flush_page
#define __arch_tlb_flush_page_lazy      i386_tlb_flush_page_lazy

/* "caches aren't brain-dead on the intel" - some clever guy */
#define __arch_cache_flush_all()        do { } while (0)
#define __arch_cache_flush_page(addr)   do { } while (0)

#endif /* ARCH_I386_RADIX_PAGE_H */
