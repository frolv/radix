/*
 * arch/i386/mm/cache.c
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

#include <radix/compiler.h>
#include <radix/cpu.h>
#include <radix/kernel.h>
#include <radix/mm.h>

static __always_inline void invlpg(addr_t addr)
{
	asm volatile("invlpg (%0)" ::"r"(addr) :"memory");
}

static __always_inline void __tlb_flush_nonglobal(void)
{
	asm volatile("movl %%cr3, %%eax;"
	             "movl %%eax, %%cr3"
	             :::"%eax");
}

static __always_inline void __tlb_flush_all(void)
{
	if (cpu_supports(CPUID_PGE)) {
		cpu_modify_cr4(CR4_PGE, 0);
		cpu_modify_cr4(0, CR4_PGE);
	} else {
		/* no global pages, so flush all == flush nonglobal */
		__tlb_flush_nonglobal();
	}
}

static void __tlb_flush_range(addr_t start, addr_t end)
{
	start = ALIGN(start, PAGE_SIZE);
	end = ALIGN(end, PAGE_SIZE);

	for (; start < end; start += PAGE_SIZE)
		invlpg(start);
}

/*
 * i386_tlb_flush_all:
 * Flush all entries in all CPUs' TLBs.
 * This function should be called only when absolutely necessary.
 */
void i386_tlb_flush_all(int sync)
{
	__tlb_flush_all();
	(void)sync;
}

/*
 * i386_tlb_flush_nonglobal:
 * Flush all non-global pages from each processor's TLB.
 */
void i386_tlb_flush_nonglobal(int sync)
{
	__tlb_flush_nonglobal();
	(void)sync;
}

/*
 * i386_tlb_flush_range:
 * Flush all pages between `start` and `end` from all processors' TLBs.
 */
void i386_tlb_flush_range(addr_t start, addr_t end, int sync)
{
	__tlb_flush_range(start, end);
	(void)sync;
}

/*
 * i386_tlb_flush_page:
 * Flush a single page from the all processors' TLBs.
 */
void i386_tlb_flush_page(addr_t addr, int sync)
{
	invlpg(addr);
	(void)sync;
}

/*
 * i386_tlb_flush_nonglobal_lazy:
 * Flush all non-global pages from the current processor's TLB.
 */
void i386_tlb_flush_nonglobal_lazy(void)
{
	__tlb_flush_nonglobal();
}

/*
 * i386_tlb_flush_range_lazy:
 * Flush all pages between `start` and `end` from the current processor's TLB.
 */
void i386_tlb_flush_range_lazy(addr_t start, addr_t end)
{
	__tlb_flush_range(start, end);
}

/*
 * i386_tlb_flush_page_lazy:
 * Flush a single page from the current processor's TLB.
 */
void i386_tlb_flush_page_lazy(addr_t addr)
{
	invlpg(addr);
}

static __always_inline void wbinvd(void)
{
	asm volatile("wbinvd");
}

void i386_cache_flush_all(void)
{
	/* TODO: this function properly */
	wbinvd();
}
