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

/*
 * The final entry in the page directory is mapped to the page directory itself.
 * The virtual address 0xFFC00000 is therefore the starting address of the page
 * directory, interpreted as a page table.
 * Virtual address 0xFFFFF000 is the page containing the actual page directory.
 */
#define PGDIR_BASE	0xFFC00000UL
#define PGDIR_VADDR	0xFFFFF000UL

#define PGTBL(x) 	(pte_t *)(PGDIR_BASE + ((x) * PAGE_SIZE))

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

/*
 * Map a page with base virtual address virt to physical address phys.
 */
int map_page(addr_t virt, addr_t phys)
{
	size_t pdi, pti;
	pte_t *pgtbl;
	addr_t pa;

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
		if (!(pa = alloc_phys_page()))
			panic("Out of memory\n");

		pgdir[pdi] = make_pde(pa | PAGE_RW | PAGE_PRESENT);
	}
	pgtbl[pti] = make_pte(phys | PAGE_RW | PAGE_PRESENT);

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
	if (!(PTE(pgtbl[pti]) & 1))
		return EINVAL;

	pgtbl[pti] = make_pte(0);

	if (freetable) {
		/* check if any other pages exist in the table */
		for (i = 0; i < PGTBL_SIZE; ++i) {
			if (PTE(pgtbl[i]) & PAGE_PRESENT)
				break;
		}
		if (i == PGTBL_SIZE) {
			free_phys_page(PDE(pgdir[pdi]));
			pgdir[pdi] = make_pde(0);
		}
	}

	return 0;
}
