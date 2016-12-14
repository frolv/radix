/*
 * arch/i386/mm/pagetable.c
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

#include <untitled/kernel.h>
#include <untitled/mm.h>
#include <untitled/page.h>

#define PGTBL(x)        (pte_t *)(PGDIR_BASE + ((x) * PAGE_SIZE))

/*
 * The page directory of a legacy 2-level x86 paging setup.
 */
pde_t * const pgdir = (pde_t *)PGDIR_VADDR;

addr_t __virt_to_phys(addr_t addr)
{
	size_t pdi, pti;
	pte_t *pgtbl;

	pdi = PGDIR_INDEX(addr);
	pti = PGTBL_INDEX(addr);

	if (PDE(pgdir[pdi]) & PAGE_PRESENT) {
		pgtbl = PGTBL(pdi);
		if (PTE(pgtbl[pti]) & PAGE_PRESENT)
			return (PTE(pgtbl[pti]) & PAGE_MASK) + (addr & 0xFFF);
	}

	return 0;
}

void __create_pgtbl(addr_t virt, pde_t pde)
{
	pgdir[PGDIR_INDEX(virt)] = pde;
}

/*
 * Map a page with base virtual address virt to physical address phys.
 */
int map_page(addr_t virt, addr_t phys)
{
	size_t pdi, pti;
	pte_t *pgtbl;
	struct page *new;

	/* addresses must be page-aligned */
	if (!ALIGNED(virt, PAGE_SIZE) || !ALIGNED(phys, PAGE_SIZE))
		return EINVAL;

	pdi = PGDIR_INDEX(virt);
	pti = PGTBL_INDEX(virt);
	pgtbl = PGTBL(pdi);

	if (PDE(pgdir[pdi]) & PAGE_PRESENT) {
		/* page is already mapped */
		if (PTE(pgtbl[pti]) & PAGE_PRESENT)
			return EINVAL;
	} else {
		/* allocate a new page table */
		new = alloc_page(PA_PAGETABLE);
		if (IS_ERR(new))
			panic("Out of memory\n");

		pgdir[pdi] = make_pde(page_to_phys(new)
		                      | PAGE_RW | PAGE_PRESENT);
	}
	pgtbl[pti] = make_pte(phys | PAGE_RW | PAGE_PRESENT);

	return 0;
}

int map_pages(addr_t virt, addr_t phys, size_t n)
{
	int err;

	for (; n; --n, virt += PAGE_SIZE, phys += PAGE_SIZE) {
		if ((err = map_page(virt, phys)) != 0)
			return err;
	}
	return 0;
}

static int __unmap(addr_t virt, int freetable);

/* Unmap the page with base address virt. */
int unmap_page(addr_t virt)
{
	return __unmap(virt, 0);
}

/*
 * Unmap the page with base address virt. If the rest of its
 * page table is empty, unmap and free the page table too.
 */
int unmap_page_pgdir(addr_t virt)
{
	return __unmap(virt, 1);
}

static int __unmap(addr_t virt, int freetable)
{
	size_t pdi, pti, i;
	pte_t *pgtbl;

	if (!ALIGNED(virt, PAGE_SIZE))
		return EINVAL;

	pdi = PGDIR_INDEX(virt);
	pti = PGTBL_INDEX(virt);

	if (!(PDE(pgdir[pdi]) & PAGE_PRESENT))
		return EINVAL;

	pgtbl = PGTBL(pdi);
	if (!(PTE(pgtbl[pti]) & PAGE_PRESENT))
		return EINVAL;

	pgtbl[pti] = make_pte(0);

	if (freetable) {
		/* check if any other pages exist in the table */
		for (i = 0; i < PGTBL_SIZE; ++i) {
			if (PTE(pgtbl[i]) & PAGE_PRESENT)
				break;
		}
		if (i == PGTBL_SIZE) {
			/* free_phys_page(PDE(pgdir[pdi])); */
			pgdir[pdi] = make_pde(0);
		}
	}

	return 0;
}
