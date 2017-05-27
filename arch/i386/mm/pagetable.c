/*
 * arch/i386/mm/pagetable.c
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

#include <radix/cpu.h>
#include <radix/kernel.h>
#include <radix/mm.h>
#include <radix/page.h>
#include <rlibc/string.h>

#define get_page_table(x) (pte_t *)(PGDIR_BASE + ((x) * PAGE_SIZE))

/* The page directory of a legacy 2-level x86 paging setup. */
pde_t * const pgdir = (pde_t *)PGDIR_VADDR;

addr_t i386_virt_to_phys(addr_t addr)
{
	size_t pdi, pti;
	pte_t *pgtbl;

	pdi = PGDIR_INDEX(addr);
	pti = PGTBL_INDEX(addr);

	if (PDE(pgdir[pdi]) & PAGE_PRESENT) {
		pgtbl = get_page_table(pdi);
		if (PTE(pgtbl[pti]) & PAGE_PRESENT)
			return (PTE(pgtbl[pti]) & PAGE_MASK) + (addr & 0xFFF);
	}

	return 0;
}

void i386_set_pde(addr_t virt, pde_t pde)
{
	pgdir[PGDIR_INDEX(virt)] = pde;
}

/*
 * i386_addr_mapped:
 * Return 1 if address `virt` has been mapped to a physical address.
 */
int i386_addr_mapped(addr_t virt)
{
	size_t pdi, pti;
	pte_t *pgtbl;

	pdi = PGDIR_INDEX(virt);
	pti = PGTBL_INDEX(virt);
	pgtbl = get_page_table(pdi);

	if (PDE(pgdir[pdi]) & PAGE_PRESENT)
		return PTE(pgtbl[pti]) & PAGE_PRESENT;
	else
		return 0;
}

/*
 * cp_to_flags:
 * Convert a cache policy to x86 page flags.
 * `flags` must already be initialized.
 */
static int cp_to_flags(unsigned long *flags, int cp)
{
	switch (cp) {
	case PAGE_CP_DEFAULT:
	case PAGE_CP_WRITE_BACK:
		*flags &= ~(PAGE_PAT | PAGE_PCD | PAGE_PWT);
		break;
	case PAGE_CP_WRITE_THROUGH:
		*flags |= PAGE_PWT;
		*flags &= ~(PAGE_PCD | PAGE_PAT);
		break;
	case PAGE_CP_UNCACHED:
		*flags |= PAGE_PCD;
		*flags &= ~(PAGE_PWT | PAGE_PAT);
		break;
	case PAGE_CP_UNCACHEABLE:
		*flags |= PAGE_PCD | PAGE_PWT;
		*flags &= ~PAGE_PAT;
		break;
	case PAGE_CP_WRITE_COMBINING:
		*flags |= PAGE_PAT;
		*flags &= ~(PAGE_PCD | PAGE_PWT);
		break;
	case PAGE_CP_WRITE_PROTECTED:
		*flags |= PAGE_PAT | PAGE_PWT;
		*flags &= ~PAGE_PCD;
		break;
	default:
		return EINVAL;
	}

	return 0;
}

static int mp_args_to_flags(unsigned long *flags, int prot, int cp)
{
	*flags = 0;

	if (prot == PROT_WRITE)
		*flags = PAGE_RW;
	else if (prot != PROT_READ)
		return EINVAL;

	return cp_to_flags(flags, cp);
}

static int __map_page(addr_t virt, addr_t phys, unsigned long flags)
{
	size_t pdi, pti;
	pte_t *pgtbl;
	struct page *new;

	/* addresses must be page-aligned */
	if (!ALIGNED(virt, PAGE_SIZE) || !ALIGNED(phys, PAGE_SIZE))
		return EINVAL;

	pdi = PGDIR_INDEX(virt);
	pti = PGTBL_INDEX(virt);
	pgtbl = get_page_table(pdi);

	if (PDE(pgdir[pdi]) & PAGE_PRESENT) {
		/* page is already mapped */
		if (PTE(pgtbl[pti]) & PAGE_PRESENT)
			return EEXIST;
	} else {
		/* allocate a new page table */
		new = alloc_page(PA_PAGETABLE);
		if (IS_ERR(new))
			return ERR_VAL(new);

		pgdir[pdi] = make_pde(page_to_phys(new)
		                      | PAGE_RW | PAGE_PRESENT);
		tlb_flush_page_lazy((addr_t)pgtbl);
		memset(pgtbl, 0, PGTBL_SIZE);
	}
	pgtbl[pti] = make_pte(phys | flags | PAGE_PRESENT);
	tlb_flush_page_lazy(virt);

	return 0;
}

/*
 * i386_map_page_kernel:
 * Map a page with base virtual address `virt` to physical address `phys`
 * for kernel use.
 */
int i386_map_page_kernel(addr_t virt, addr_t phys, int prot, int cp)
{
	unsigned long flags;
	int err;

	if ((err = mp_args_to_flags(&flags, prot, cp)) != 0)
		return err;

	return __map_page(virt, phys, flags | PAGE_GLOBAL);
}

/*
 * i386_map_page_user:
 * Map a page with base virtual address `virt` to physical address `phys`
 * for userspace.
 */
int i386_map_page_user(addr_t virt, addr_t phys, int prot, int cp)
{
	unsigned long flags;
	int err;

	if ((err = mp_args_to_flags(&flags, prot, cp)) != 0)
		return err;

	return __map_page(virt, phys, flags | PAGE_USER);
}

int i386_map_pages(addr_t virt, addr_t phys, int prot,
                   int cp, int user, size_t n)
{
	unsigned long flags;
	int err;

	if ((err = mp_args_to_flags(&flags, prot, cp)) != 0)
		return err;

	flags |= user ? PAGE_USER : PAGE_GLOBAL;

	for (; n; --n, virt += PAGE_SIZE, phys += PAGE_SIZE) {
		if ((err = __map_page(virt, phys, flags)) != 0)
			return err;
	}

	return 0;
}

static int __unmap(addr_t virt, int freetable);

/* i386_unmap_page: unmap the page with base address `virt` */
int i386_unmap_page(addr_t virt)
{
	return __unmap(virt, 0);
}

/*
 * unmap_page_clean:
 * Unmap the page with base address `virt`. If the rest of its
 * page table is empty, unmap and free the page table too.
 */
int i386_unmap_page_clean(addr_t virt)
{
	return __unmap(virt, 1);
}

static int __unmap(addr_t virt, int freetable)
{
	size_t pdi, pti, i;
	pte_t *pgtbl;
	addr_t phys;

	if (!ALIGNED(virt, PAGE_SIZE))
		return EINVAL;

	pdi = PGDIR_INDEX(virt);
	pti = PGTBL_INDEX(virt);

	if (!(PDE(pgdir[pdi]) & PAGE_PRESENT))
		return EINVAL;

	pgtbl = get_page_table(pdi);
	if (!(PTE(pgtbl[pti]) & PAGE_PRESENT))
		return EINVAL;

	pgtbl[pti] = make_pte(0);
	tlb_flush_page_lazy(virt);

	if (freetable) {
		/* check if any other pages exist in the table */
		for (i = 0; i < PTRS_PER_PGTBL; ++i) {
			if (PTE(pgtbl[i]) & PAGE_PRESENT)
				break;
		}
		if (i == PTRS_PER_PGTBL) {
			phys = PDE(pgdir[pdi]) & PAGE_MASK;
			free_pages(phys_to_page(phys));
			pgdir[pdi] = make_pde(0);
			tlb_flush_page_lazy((addr_t)pgtbl);
		}
	}

	return 0;
}

/*
 * i386_set_cache_policy:
 * Set the CPU caching policy for a single virtual page.
 * TODO: flush caches
 */
int i386_set_cache_policy(addr_t virt, enum cache_policy policy)
{
	size_t pdi, pti;
	pte_t *pgtbl;
	unsigned long pte;
	int err;

	pdi = PGDIR_INDEX(virt);
	pti = PGTBL_INDEX(virt);

	if (!(PDE(pgdir[pdi]) & PAGE_PRESENT))
		return EINVAL;

	pgtbl = get_page_table(pdi);
	pte = PTE(pgtbl[pti]);

	if (!(pte & PAGE_PRESENT))
		return EINVAL;

	/* WC and WP cache policies are only available through PAT. */
	if (!cpu_supports(CPUID_PAT) &&
	    (policy == PAGE_CP_WRITE_COMBINING ||
	     policy == PAGE_CP_WRITE_PROTECTED))
		policy = PAGE_CP_WRITE_BACK;

	if ((err = cp_to_flags(&pte, policy)) != 0)
		return err;

	pgtbl[pti] = make_pte(pte);
	tlb_flush_page_lazy(virt);

	return 0;
}
