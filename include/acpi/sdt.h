/*
 * include/acpi/sdt.h
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

#ifndef ACPI_SDT_H
#define ACPI_SDT_H

#include <untitled/types.h>

/* Header of an ACPI System Description Table. */
struct acpi_sdt_header {
	char            signature[4];
	uint32_t        length;
	uint8_t         revision;
	uint8_t         checksum;
	char            oem_id[6];
	char            oem_table_id[8];
	uint32_t        oem_revision;
	uint32_t        creator_id;
	uint32_t        creator_revision;
};

int acpi_valid_checksum(struct acpi_sdt_header *header);

#endif /* ACPI_SDT_H */
