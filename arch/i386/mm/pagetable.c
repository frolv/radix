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

#include <errno.h>
#include <untitled/kernel.h>
#include <untitled/mm.h>
#include <untitled/page.h>

/*
 * The final entry in the page directory is mapped to the page directory itself.
 * The virtual address 0xFFC00000 is therefore the starting address of the page
 * directory, interpreted as a page table.
 * Virtual address 0xFFFFF000 is the page containing the actual page directory.
 */
#define PGDIR_BASE	0xFFC00000
#define PGDIR_VADDR	0xFFFFF000

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

	/* ensure addresses are page-aligned */
	virt = ALIGN(virt, PAGE_SIZE);
	phys = ALIGN(phys, PAGE_SIZE);

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
