/*
 * arch/i386/cpu/pat.c
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

#include <radix/asm/msr.h>
#include <radix/cpu.h>

enum {
	PAT_0,
	PAT_1,
	PAT_2,
	PAT_3,
	PAT_4,
	PAT_5,
	PAT_6,
	PAT_7
};

#define PAT_UC          0x00    /* uncacheable */
#define PAT_WC          0x01    /* write combining */
#define PAT_WT          0x04    /* write through */
#define PAT_WP          0x05    /* write protected */
#define PAT_WB          0x06    /* write back */
#define PAT_UCMINUS     0x07    /* uncached */

#define pat_set_lo(reg, val) (((val) & 0xFF) << ((reg) * 8))
#define pat_set_hi(reg, val) (((val) & 0xFF) << (((reg) - 4) * 8))

/*
 * pat_init:
 * Initialize the Page Attribute Table fields.
 */
int pat_init(void)
{
	uint32_t lo, hi;

	if (!cpu_supports(CPUID_PAT))
		return 1;

	/*
	 * Set the first four PAT entries to be compatible with
	 * the PWT/PCD page bits for legacy caching control.
	 */
	lo = pat_set_lo(PAT_0, PAT_WB)
		| pat_set_lo(PAT_1, PAT_WT)
		| pat_set_lo(PAT_2, PAT_UCMINUS)
		| pat_set_lo(PAT_3, PAT_UC);

	/*
	 * PAT_4 and PAT_5 are used for the two new cache types.
	 * The values of PAT_6 and PAT_7 are irrelevant; they're never used.
	 */
	hi = pat_set_hi(PAT_4, PAT_WC)
		| pat_set_hi(PAT_5, PAT_WP)
		| pat_set_hi(PAT_6, PAT_UC)
		| pat_set_hi(PAT_7, PAT_UC);

	wrmsr(IA32_PAT, lo, hi);

	return 0;
}
