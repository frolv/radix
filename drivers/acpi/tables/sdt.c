/*
 * drivers/acpi/tables/sdt.c
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

#include <acpi/acpi.h>
#include <acpi/tables/sdt.h>

int acpi_valid_checksum(struct acpi_sdt_header *header)
{
	size_t i;
	int sum;

	if (header->length > 0x800)
		return 0;

	sum = 0;
	for (i = 0; i < header->length; ++i)
		sum += ((char *)header)[i];

	return (sum & 0xFF) == 0;
}
