/*
 * arch/i386/cpu/gdt.h
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

#ifndef ARCH_I386_GDT_H
#define ARCH_I386_GDT_H

enum {
	GDT_NULL,
	GDT_KERNEL_CODE,
	GDT_KERNEL_DATA,
	GDT_USER_CODE,
	GDT_USER_DATA,
	GDT_TSS,
	GDT_FS,
	GDT_GS
};

#define GDT_DESCRIPTOR_SIZE 8
#define GDT_OFFSET(desc) ((desc) * GDT_DESCRIPTOR_SIZE)

#include <radix/types.h>

void gdt_init(void);
void gdt_set_fsbase(uint32_t base);
void gdt_set_gsbase(uint32_t base);

#endif /* ARCH_I386_GDT_H */
