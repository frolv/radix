/*
 * include/radix/asm/io.h
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

#ifndef RADIX_ASM_IO_H
#define RADIX_ASM_IO_H

#include <radix/asm/arch_io.h>

#define outb __arch_outb
#define outw __arch_outw
#define outl __arch_outl

#define inb __arch_inb
#define inw __arch_inw
#define inl __arch_inl

#define io_wait __arch_io_wait

#endif /* RADIX_ASM_IO_H */
