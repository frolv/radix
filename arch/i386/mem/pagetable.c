/*
 * arch/i386/mem/pagetable.c
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

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <untitled/compiler.h>
#include <untitled/page.h>

/*
 * The page directory of a legacy 2-level x86 paging setup.
 */
pde_t pgdir[PGDIR_SIZE] __aligned(0x1000);

/* An initial page table to be used by the kernel on boot. */
static pte_t base_page_table[PGTBL_SIZE] __aligned(0x1000);

extern void pgdir_load(uintptr_t pgdir);

void init_page_directory(void)
{
	size_t i;
	const pdeval_t flags = PAGE_RW | PAGE_PRESENT;

	memset(pgdir, 0, sizeof pgdir);

	/*
	 * Map the first 1 MiB of virtual memory to the
	 * first 1 MiB of physical memory.
	 */
	for (i = 0; i < PGTBL_SIZE; ++i)
		base_page_table[i] = make_pte((i * 0x1000) | flags);

	pgdir[0] = make_pde(((uintptr_t)base_page_table) | flags);

	pgdir_load((uintptr_t)pgdir);
}
