/*
 * arch/i386/include/radix/asm/mm_limits.h
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

#ifndef ARCH_I386_RADIX_MM_LIMITS_H
#define ARCH_I386_RADIX_MM_LIMITS_H

#if !defined(RADIX_MM_H) && !defined(RADIX_VMM_H)
#error <radix/asm/mm_limits.h> cannot be included directly
#endif

/* Pages containing ACPI tables are mapped starting at this virtual address. */
#define __ARCH_ACPI_VIRT_BASE     0xFFBF0000UL

/* 16 reserved pages for up to 16 I/O APICs */
#define __ARCH_IOAPIC_VIRT_BASE   0xFFBE0000UL
/* Local APIC register page */
#define __ARCH_APIC_VIRT_PAGE     0xFFBDF000UL

/* Kernel available virtual address range */
#define __ARCH_KERNEL_VIRT_BASE   0xC0000000UL
#define __ARCH_RESERVED_VIRT_BASE 0xD0000000UL

#define __ARCH_MEM_LIMIT          0x100000000ULL

#define __ARCH_PGDIR_BASE         0xFFC00000UL
#define __ARCH_PGDIR_VADDR        0xFFFFF000UL

#endif /* ARCH_I386_RADIX_MM_LIMITS_H */
