/*
 * include/acpi/tables/hpet.h
 * Copyright (C) 2017 Alexei Frolov
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

#ifndef ACPI_TABLES_HPET_H
#define ACPI_TABLES_HPET_H

#include <radix/types.h>

#include <acpi/tables/gas.h>
#include <acpi/tables/sdt.h>

#define ACPI_HPET_SIGNATURE "HPET"

struct acpi_hpet {
    struct acpi_sdt_header header;
    uint32_t timer_blk_id;
    struct acpi_generic_address hpet_base;
    uint8_t hpet_number;
    uint16_t min_periodic_ticks;
    uint8_t page_prot_oem_attr;
};

#endif /* ACPI_TABLES_HPET_H */
