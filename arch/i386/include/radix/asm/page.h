/*
 * arch/i386/include/radix/asm/page.h
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

#ifndef ARCH_I386_RADIX_PAGE_H
#define ARCH_I386_RADIX_PAGE_H

#if !defined(RADIX_MM_H) && !defined(RADIX_VMM_H)
#error <radix/asm/page.h> cannot be included directly
#endif

#include <radix/asm/mm_types.h>
#include <radix/asm/mm_limits.h>
#include <radix/asm/page_defs.h>
#include <radix/compiler.h>
#include <radix/config.h>

#define PDE(x) ((x).pde)
#define PTE(x) ((x).pte)

#if CONFIG(X86_PAE)
#define PDPTE(x) ((x).pdpte)
#endif  // CONFIG(X86_PAE)

static __always_inline pde_t make_pde(pdeval_t val)
{
	return (pde_t){ val };
}

static __always_inline pte_t make_pte(pteval_t val)
{
	return (pte_t){ val };
}

#if CONFIG(X86_PAE)
static __always_inline pdpte_t make_pdpte(pdpteval_t val)
{
	return (pdpte_t){ val };
}
#endif  // CONFIG(X86_PAE)

#include <radix/types.h>

enum cache_policy;
struct vmm_space;

/*
 * i386 definitions of generic memory management functions.
 */
paddr_t i386_virt_to_phys(addr_t addr);
void i386_set_pde(addr_t virt, pde_t pde);
int i386_addr_mapped(addr_t virt);
int i386_map_page_kernel(addr_t virt, paddr_t phys, int prot, int cp);
int i386_map_page_user(addr_t virt, paddr_t phys, int prot, int cp);
int i386_map_pages(addr_t virt, paddr_t phys, int prot,
                   int cp, int user, size_t n);
int i386_unmap_pages(addr_t virt, size_t n);
int i386_set_cache_policy(addr_t virt, enum cache_policy policy);

void i386_tlb_flush_all(int sync);
void i386_tlb_flush_nonglobal(int sync);
void i386_tlb_flush_nonglobal_lazy(void);
void i386_tlb_flush_range(addr_t start, addr_t end, int sync);
void i386_tlb_flush_range_lazy(addr_t start, addr_t end);
void i386_tlb_flush_page(addr_t addr, int sync);
void i386_tlb_flush_page_lazy(addr_t addr);

void i386_switch_address_space(struct vmm_space *vmm);

static __always_inline addr_t __arch_pa(addr_t v)
{
	if (v < __ARCH_KERNEL_VIRT_BASE || v >= __ARCH_RESERVED_VIRT_BASE)
		return i386_virt_to_phys(v);
	else
		return v - __ARCH_KERNEL_VIRT_BASE;
}

#define __arch_va(addr) ((addr) + __ARCH_KERNEL_VIRT_BASE)

#define __arch_set_pde                  i386_set_pde
#define __arch_addr_mapped              i386_addr_mapped
#define __arch_map_page_kernel          i386_map_page_kernel
#define __arch_map_page_user            i386_map_page_user
#define __arch_map_pages                i386_map_pages
#define __arch_unmap_pages              i386_unmap_pages
#define __arch_set_cache_policy         i386_set_cache_policy
#define __arch_switch_address_space     i386_switch_address_space

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
