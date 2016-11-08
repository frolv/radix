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

#include <untitled/kernel.h>
#include <untitled/mm.h>
#include <untitled/page.h>

#include "physmem.h"

/* total amount of usable memory in the system */
uint64_t totalmem = 0;

#define KERNEL_PHYSICAL_END	0x00800000UL

#define make64(low, high) ((uint64_t)(high) << 32 | (low))

void detect_memory(multiboot_info_t *mbt)
{
	memory_map_t *mmap;
	uint64_t base, orig_base, len;

	phys_stack_init();

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

		base = make64(mmap->base_addr_low, mmap->base_addr_high);
		len = make64(mmap->length_low, mmap->length_high);

		/*
		 * This should already be aligned by the bootloader.
		 * But, just in case...
		 */
		orig_base = base;
		base = ALIGN(base, PAGE_SIZE);
		len = (len - (base - orig_base)) & PAGE_MASK;
		if (!len)
			continue;

		totalmem += len;

		/*
		 * The first 4 MiB of physical memory is reserved
		 * for the bootloader and the kernel. The next 4 MiB
		 * is reserved for the physical memory stack.
		 */
		if (base < KERNEL_PHYSICAL_END) {
			if (base + len <= KERNEL_PHYSICAL_END)
				continue;
			len = len - (KERNEL_PHYSICAL_END - base);
			base = KERNEL_PHYSICAL_END;
		}

		mark_free_region(base, len);
	}
}
