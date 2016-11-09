/*
 * include/untitled/mm.h
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

#ifndef UNTITLED_MM_H
#define UNTITLED_MM_H

#include <stddef.h>
#include <stdint.h>
#include <untitled/mm_types.h>
#include <untitled/multiboot.h>
#include <untitled/page.h>

#define KERNEL_VIRTUAL_BASE	0xC0000000UL

#define phys_addr(x) __pa(x)

extern uint64_t totalmem;

void detect_memory(multiboot_info_t *mbt);

addr_t alloc_phys_page();
void free_phys_page(addr_t base);

int map_page(addr_t virt, addr_t phys);
int unmap_page(addr_t virt);

#endif /* UNTITLED_MM_H */
