/*
 * arch/i386/include/radix/asm/mm_limits.h
 * Copyright (C) 2021 Alexei Frolov
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

#include <radix/config.h>

#if !defined(RADIX_MM_H) && !defined(RADIX_VMM_H)
#error <radix/asm/mm_limits.h> cannot be included directly
#endif

// Base virtual address at which kernel code is loaded.
#define __ARCH_KERNEL_VIRT_BASE 0xC0000000UL

// Base virtual address for the kernel's dynamic address space.
#define __ARCH_RESERVED_VIRT_BASE 0xD0000000UL

// Virtual address range for user processes.
#define __ARCH_USER_VIRT_BASE 0x0100000UL
#define __ARCH_USER_VIRT_SIZE (__ARCH_KERNEL_VIRT_BASE - __ARCH_USER_VIRT_BASE)

#if CONFIG(X86_PAE)

#define __ARCH_PAGING_BASE  0xFF600000UL
#define __ARCH_PAGING_VADDR 0xFF600000UL
#define __ARCH_MEM_LIMIT    0x1000000000ULL

#else  // CONFIG(X86_PAE)

#define __ARCH_PAGING_BASE  0xFFC00000UL
#define __ARCH_PAGING_VADDR 0xFFFFF000UL
#define __ARCH_MEM_LIMIT    0x100000000ULL

#endif  // CONFIG(X86_PAE)

#endif  // ARCH_I386_RADIX_MM_LIMITS_H
