/*
 * arch/i386/mm/physmem.c
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

#include <untitled/compiler.h>
#include <untitled/kernel.h>
#include <untitled/mm.h>
#include <untitled/page.h>

#include "physmem.h"

/*
 * The physical address stack occupies a 4 MiB in memory,
 * from virtual addresses 0xC0400000 to 0xC07FFFFF.
 */
#define STACK_BASE	0xC0400000
#define MAX_STACK_SIZE	(4UL * 0x400 * 0x400)
#define MAX_STACK_LEN	(MAX_STACK_SIZE / sizeof (addr_t))

static addr_t * const phys_stack = (addr_t *)STACK_BASE;
static size_t stack_length;

/* page table to map the stack's addresses */
static pte_t stack_pgtbl[PGTBL_SIZE] __aligned(0x1000);

/* system page directory */
extern pde_t *pgdir;

void phys_stack_init(void)
{
	size_t i;
	const pdeval_t flags = PAGE_RW | PAGE_PRESENT;
	/*
	 * Physical base address of the stack.
	 * Occupies the 4 MiB in memory directory after the kernel.
	 */
	const addr_t base = STACK_BASE - KERNEL_VIRTUAL_BASE;

	for (i = 0; i < PGTBL_SIZE; ++i)
		stack_pgtbl[i] = make_pte((base + (i * PAGE_SIZE)) | flags);

	/* entry 0x301 maps virtual addresses 0xC0400000 through 0xC07FFFFF */
	pgdir[0x301] = make_pde(phys_addr((uintptr_t)stack_pgtbl) | flags);

	stack_length = 0;
}

/*
 * Mark a region of physical memory as free by pushing it onto the stack.
 * If the region does not start at a page boundary, it is aligned up to
 * the next page.
 */
void mark_free_region(addr_t base, size_t len)
{
	addr_t pos;

	pos = ALIGN(base, PAGE_SIZE);
	len = (len - (pos - base)) & PAGE_MASK;

	while (len) {
		if (unlikely(stack_length == MAX_STACK_LEN)) {
			/* do something */
			return;
		}
		phys_stack[stack_length++] = pos;

		len -= PAGE_SIZE;
		pos += PAGE_SIZE;
	}
}

addr_t alloc_phys_page(void)
{
	return phys_stack[--stack_length];
}

void free_phys_page(addr_t base)
{
	mark_free_region(base, PAGE_SIZE);
}
