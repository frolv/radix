/*
 * arch/i386/include/untitled/page.h
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

#ifndef ARCH_I386_UNTITLED_PAGE_H
#define ARCH_I386_UNTITLED_PAGE_H

typedef unsigned long pdeval_t;
typedef unsigned long pteval_t;

typedef struct {
	pdeval_t pde;
} pde_t;

typedef struct {
	pteval_t pte;
} pte_t;

#define PGDIR_SIZE		0x400
#define PGTBL_SIZE		0x400

#define _PAGE_BIT_PRESENT	0
#define _PAGE_BIT_RW		1
#define _PAGE_BIT_USER		2
#define _PAGE_BIT_WT		3
#define _PAGE_BIT_CD		4
#define _PAGE_BIT_ACCESSED	5
#define _PAGE_BIT_DIRTY		6
#define _PAGE_BIT_SIZE		7
#define _PAGE_BIT_GLOBAL	8

#define PAGE_PRESENT	(((pteval_t)1) << _PAGE_BIT_PRESENT)
#define PAGE_RW		(((pteval_t)1) << _PAGE_BIT_RW)
#define PAGE_USER	(((pteval_t)1) << _PAGE_BIT_USER)
#define PAGE_WT		(((pteval_t)1) << _PAGE_BIT_WT)
#define PAGE_CD		(((pteval_t)1) << _PAGE_BIT_CD)
#define PAGE_ACCESSED	(((pteval_t)1) << _PAGE_BIT_ACCESSED)
#define PAGE_DIRTY	(((pteval_t)1) << _PAGE_BIT_DIRTY)
#define PAGE_SIZE	(((pteval_t)1) << _PAGE_BIT_SIZE)
#define PAGE_GLOBAL	(((pteval_t)1) << _PAGE_BIT_GLOBAL)

static __always_inline pde_t make_pde(pdeval_t val)
{
	return (pde_t){ val };
}

static __always_inline pte_t make_pte(pteval_t val)
{
	return (pte_t){ val };
}

#endif /* ARCH_I386_UNTITLED_PAGE_H */
