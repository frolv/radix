/*
 * arch/i386/cpu/cpu.c
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

#include <string.h>
#include <untitled/cpu.h>

static char vendor_id[16];

void read_cpu_info(void)
{
	int eax;
	int vendor[4];

	if (!cpuid_supported()) {
		vendor_id[0] = '\0';
		return;
	}

	cpuid(0, eax, vendor[0], vendor[2], vendor[1]);
	vendor[3] = 0;

	memcpy(vendor_id, vendor, sizeof vendor_id);
}
