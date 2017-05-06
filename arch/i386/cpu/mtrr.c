/*
 * arch/i386/cpu/mtrr.c
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

#define IA32_MTRRCAP_VCNT 0xFF          /* variable MTRR count */
#define IA32_MTRRCAP_FIX  (1 << 8)      /* fixed range registers */
#define IA32_MTRRCAP_WC   (1 << 10)     /* write-combining memory */
#define IA32_MTRRCAP_SMRR (1 << 11)     /* SMRR interface */

int mtrr_variable_range_count(void)
{
	uint32_t lo, hi;

	rdmsr(IA32_MTRRCAP, &lo, &hi);
	return lo & IA32_MTRRCAP_VCNT;
}
