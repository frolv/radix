/*
 * arch/i386/include/radix/page.h
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

#define PTRS_PER_PGDIR          0x400
#define PTRS_PER_PGTBL          0x400
#define	PGDIR_SIZE              (PTRS_PER_PGDIR * sizeof (void *))
#define	PGTBL_SIZE              (PTRS_PER_PGTBL * sizeof (void *))

#define PGDIR_SHIFT             22
#define PAGE_SHIFT              12
#define PAGE_SIZE               (1UL << PAGE_SHIFT)
#define PAGE_MASK               (~(PAGE_SIZE - 1))

#define PGDIR_INDEX(x)          ((x) >> PGDIR_SHIFT)
#define PGTBL_INDEX(x)          (((x) >> PAGE_SHIFT) & 0x3FF)

#define _PAGE_BIT_PRESENT       0
#define _PAGE_BIT_RW            1
#define _PAGE_BIT_USER          2
#define _PAGE_BIT_WT            3
#define _PAGE_BIT_CD            4
#define _PAGE_BIT_ACCESSED      5
#define _PAGE_BIT_DIRTY         6
#define _PAGE_BIT_PSE           7
#define _PAGE_BIT_GLOBAL        8

#define PAGE_PRESENT    (((pteval_t)1) << _PAGE_BIT_PRESENT)
#define PAGE_RW         (((pteval_t)1) << _PAGE_BIT_RW)
#define PAGE_USER       (((pteval_t)1) << _PAGE_BIT_USER)
#define PAGE_WT         (((pteval_t)1) << _PAGE_BIT_WT)
#define PAGE_CD         (((pteval_t)1) << _PAGE_BIT_CD)
#define PAGE_ACCESSED   (((pteval_t)1) << _PAGE_BIT_ACCESSED)
#define PAGE_DIRTY      (((pteval_t)1) << _PAGE_BIT_DIRTY)
#define PAGE_PSE        (((pteval_t)1) << _PAGE_BIT_PSE)
#define PAGE_GLOBAL     (((pteval_t)1) << _PAGE_BIT_GLOBAL)

#include <radix/compiler.h>

#include <radix/mm_types.h>

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

/*
 * The final entry in the page directory is mapped to the page directory itself.
 * The virtual address 0xFFC00000 is therefore the starting address of the page
 * directory, interpreted as a page table.
 * Virtual address 0xFFFFF000 is the page containing the actual page directory.
 */
#define PGDIR_BASE      0xFFC00000UL
#define PGDIR_VADDR     0xFFFFF000UL

addr_t __virt_to_phys(addr_t addr);

static __always_inline addr_t __pa(addr_t v)
{
	if (v < __ARCH_KERNEL_VIRT_BASE || v >= __ARCH_RESERVED_VIRT_BASE)
		return __virt_to_phys(v);
	else
		return v - __ARCH_KERNEL_VIRT_BASE;
}

#endif /* ARCH_I386_RADIX_PAGE_H */
