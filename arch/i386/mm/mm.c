/*
 * arch/i386/mm/mm.c
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

#include <untitled/mm.h>

/* total amount of useable memory in the system */
uint64_t totalmem = 0;

#define KERNEL_VIRTUAL_BASE 0xC0000000

#define make64(low, high) ((uint64_t)(high) << 32 | (low))

void detect_memory(multiboot_info_t *mbt)
{
	memory_map_t *mmap;
	uint64_t curr_len;
	uint64_t curr_addr;

	/*
	 * mmap_addr stores the physical address of the memory map.
	 * Make it virtual.
	 */
	mbt->mmap_addr += KERNEL_VIRTUAL_BASE;

	for (mmap = (memory_map_t *)mbt->mmap_addr;
			(uintptr_t)mmap < mbt->mmap_addr + mbt->mmap_length;
			mmap = (memory_map_t *)((uintptr_t)mmap + mmap->size
						+ sizeof (mmap->size))) {
		/* only consider available RAM */
		if (mmap->type != 1)
			continue;

		curr_addr = make64(mmap->base_addr_low, mmap->base_addr_high);
		curr_len = make64(mmap->length_low, mmap->length_high);
		totalmem += curr_len;
	}
}
