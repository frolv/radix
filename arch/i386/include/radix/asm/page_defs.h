/*
 * arch/i386/include/radix/asm/page_defs.h
 * Copyright (C) 2017 Alexei Frolov
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

#ifndef ARCH_I386_RADIX_PAGE_DEFS_H
#define ARCH_I386_RADIX_PAGE_DEFS_H

#ifdef CONFIG_PAE
#define PTRS_PER_PDPT           0x004
#define PTRS_PER_PGDIR          0x200
#define PTRS_PER_PGTBL          0x200
#else
#define PTRS_PER_PGDIR          0x400
#define PTRS_PER_PGTBL          0x400
#endif

#define PGDIR_SIZE              (PTRS_PER_PGDIR * sizeof (pde_t))
#define PGTBL_SIZE              (PTRS_PER_PGTBL * sizeof (pte_t))

#define PAGE_SHIFT              12
#define PAGE_SIZE               (1U << PAGE_SHIFT)
#define PAGE_MASK               (~(PAGE_SIZE - 1))

#ifdef CONFIG_PAE
#define PDPT_SHIFT              30
#define PGDIR_SHIFT             21

#define PDPT_INDEX(x)           ((x) >> PDPT_SHIFT)
#define PGDIR_INDEX(x)          (((x) >> PGDIR_SHIFT) & 0x1FF)
#define PGTBL_INDEX(x)          (((x) >> PAGE_SHIFT) & 0x1FF)
#else
#define PGDIR_SHIFT             22

#define PGDIR_INDEX(x)          ((x) >> PGDIR_SHIFT)
#define PGTBL_INDEX(x)          (((x) >> PAGE_SHIFT) & 0x3FF)
#endif /* CONFIG_PAE */

#define _PAGE_BIT_PRESENT       0
#define _PAGE_BIT_RW            1
#define _PAGE_BIT_USER          2
#define _PAGE_BIT_PWT           3
#define _PAGE_BIT_PCD           4
#define _PAGE_BIT_ACCESSED      5
#define _PAGE_BIT_DIRTY         6
#define _PAGE_BIT_PAT           7
#define _PAGE_BIT_GLOBAL        8
#ifdef CONFIG_PAE
#define _PAGE_BIT_NX            63
#endif

#define PAGE_PRESENT            (((pteval_t)1) << _PAGE_BIT_PRESENT)
#define PAGE_RW                 (((pteval_t)1) << _PAGE_BIT_RW)
#define PAGE_USER               (((pteval_t)1) << _PAGE_BIT_USER)
#define PAGE_PWT                (((pteval_t)1) << _PAGE_BIT_PWT)
#define PAGE_PCD                (((pteval_t)1) << _PAGE_BIT_PCD)
#define PAGE_ACCESSED           (((pteval_t)1) << _PAGE_BIT_ACCESSED)
#define PAGE_DIRTY              (((pteval_t)1) << _PAGE_BIT_DIRTY)
#define PAGE_PAT                (((pteval_t)1) << _PAGE_BIT_PAT)
#define PAGE_GLOBAL             (((pteval_t)1) << _PAGE_BIT_GLOBAL)
#ifdef CONFIG_PAE
#define PAGE_NX                 (((pteval_t)1) << _PAGE_BIT_NX)
#endif

#endif /* ARCH_I386_RADIX_PAGE_DEFS_H */
