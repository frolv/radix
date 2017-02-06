/*
 * drivers/acpi/acpi.c
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
#include <acpi/acpi.h>
#include <acpi/rsdp.h>
#include <untitled/mm.h>

#define RSDP_SIG "RSD PTR "

#define BIOS_REGION_PHYS_START  0x000E0000
#define BIOS_REGION_PHYS_END    0x00100000

#define BIOS_REGION_START       (BIOS_REGION_PHYS_START + KERNEL_VIRTUAL_BASE)
#define BIOS_REGION_END         (BIOS_REGION_PHYS_END + KERNEL_VIRTUAL_BASE)

#if defined(__i386__)
static struct acpi_rsdp *rsdp;
#elif defined(__x86_64__)
static struct acpi_rsdp_2 *rsdp;
#endif

void acpi_init(void)
{
	uint64_t *s;

	/*
	 * The RSDP descriptor is located within the main BIOS memory region.
	 * It is identified by the signature "RSD PTR ", aligned on a 16-byte
	 * boundary.
	 */
	s = (void *)BIOS_REGION_START;
	for (; s < (uint64_t *)BIOS_REGION_END; s += 2) {
		if (memcmp(s, RSDP_SIG, sizeof *s) == 0) {
			rsdp = (void *)s;
			break;
		}
	}
}

void *acpi_find_table(char *signature)
{
	return NULL;
}
