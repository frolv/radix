/*
 * include/acpi/tables/fadt.h
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

#ifndef ACPI_TABLES_FADT_H
#define ACPI_TABLES_FADT_H

#include <acpi/tables/sdt.h>
#include <radix/types.h>

#define ACPI_FADT_SIGNATURE "FACP"

struct acpi_generic_address {
	uint8_t         address_space;
	uint8_t         bit_width;
	uint8_t         bit_offset;
	uint8_t         access_size;
	uint64_t        address;
};

struct acpi_fadt {
	struct acpi_sdt_header header;
	uint32_t        firmware_ctrl;
	uint32_t        dsdt;
	uint8_t         reserved;
	uint8_t         preferred_pm_profile;
	uint16_t        sci_int;
	uint32_t        smi_cmd;
	uint8_t         acpi_enable;
	uint8_t         acpi_disable;
	uint8_t         s4bios_req;
	uint8_t         pstate_cnt;
	uint32_t        pm1a_evt_blk;
	uint32_t        pm1b_evt_blk;
	uint32_t        pm1a_cnt_blk;
	uint32_t        pm1b_cnt_blk;
	uint32_t        pm2_cnt_blk;
	uint32_t        pm_tmr_blk;
	uint32_t        gpe0_blk;
	uint32_t        gpe1_blk;
	uint8_t         pm1_evt_len;
	uint8_t         pm1_cnt_len;
	uint8_t         pm2_cnt_len;
	uint8_t         pm_tmr_len;
	uint8_t         gpe0_blk_len;
	uint8_t         gpe1_blk_len;
	uint8_t         gpe1_base;
	uint8_t         cst_cnt;
	uint16_t        p_lvl2_lat;
	uint16_t        p_lvl3_lat;
	uint16_t        flush_size;
	uint16_t        flush_stride;
	uint8_t         duty_offset;
	uint8_t         duty_width;
	uint8_t         day_alrm;
	uint8_t         mon_alrm;
	uint8_t         century;
	uint16_t        iapc_boot_arch;
	uint8_t         reserved2;
	uint32_t        flags;
	struct acpi_generic_address reset_reg;
	uint8_t         reset_value;
	uint8_t         reserved3[3];
	struct acpi_generic_address x_firmware_ctrl;
	struct acpi_generic_address x_dsdt;
	struct acpi_generic_address x_pm1a_evt_blk;
	struct acpi_generic_address x_pm1b_evt_blk;
	struct acpi_generic_address x_pm1a_cnt_blk;
	struct acpi_generic_address x_pm1b_cnt_blk;
	struct acpi_generic_address x_pm2_cnt_blk;
	struct acpi_generic_address x_pm_tmr_blk;
	struct acpi_generic_address x_gpe0_blk;
	struct acpi_generic_address x_gpe1_blk;
	struct acpi_generic_address sleep_control_reg;
	struct acpi_generic_address sleep_status_reg;
};

#endif /* ACPI_TABLES_FADT_H */
