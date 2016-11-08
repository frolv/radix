/*
 * arch/i386/mm/physmem.h
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

#ifndef ARCH_I386_MM_PHYSMEM_H
#define ARCH_I386_MM_PHYSMEM_H

#include <stddef.h>
#include <untitled/mm_types.h>

void phys_stack_init(void);

void mark_free_region(addr_t base, size_t len);

addr_t alloc_phys_page(void);
void free_phys_page(addr_t base);

#endif /* ARCH_I386_MM_PHYSMEM_H */
