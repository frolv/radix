/*
 * arch/i386/include/radix/asm/mm_types.h
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

#ifndef ARCH_I386_RADIX_MM_TYPES_H
#define ARCH_I386_RADIX_MM_TYPES_H

#ifndef RADIX_MM_TYPES_H
#error only <radix/mm_types.h> can be included directly
#endif

#include <radix/asm/page_defs.h>
#include <radix/config.h>

#include <stdint.h>

#if CONFIG(X86_PAE)

typedef unsigned long addr_t;
typedef uint64_t paddr_t;

typedef uint64_t pdeval_t;
typedef uint64_t pteval_t;
typedef uint64_t pdpteval_t;

#else  // CONFIG(X86_PAE)

typedef unsigned long addr_t;
typedef unsigned long paddr_t;

typedef unsigned long pdeval_t;
typedef unsigned long pteval_t;

#endif  // CONFIG(X86_PAE)

typedef struct {
    pdeval_t pde;
} pde_t;

typedef struct {
    pteval_t pte;
} pte_t;

#if CONFIG(X86_PAE)

typedef struct {
    pdpteval_t pdpte;
} pdpte_t;

#define PDPT_ENTRY_00 0
#define PDPT_ENTRY_40 1
#define PDPT_ENTRY_80 2
#define PDPT_ENTRY_C0 3

struct pdpt {
    pdpte_t entries[PTRS_PER_PDPT];
};

#endif  // CONFIG(X86_PAE)

#endif  // ARCH_I386_RADIX_MM_TYPES_H
