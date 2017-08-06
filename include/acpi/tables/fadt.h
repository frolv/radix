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

#include <acpi/tables/gas.h>
#include <acpi/tables/sdt.h>
#include <radix/types.h>

#define ACPI_FADT_SIGNATURE "FACP"

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

#define ACPI_FADT_WBINVD                        (1 << 0)
#define ACPI_FADT_WBINVD_FLUSH                  (1 << 1)
#define ACPI_FADT_PROC_C1                       (1 << 2)
#define ACPI_FADT_P_LVL2_UP                     (1 << 3)
#define ACPI_FADT_PWR_BUTTON                    (1 << 4)
#define ACPI_FADT_SLP_BUTTON                    (1 << 5)
#define ACPI_FADT_FIX_RTC                       (1 << 6)
#define ACPI_FADT_RTC_S4                        (1 << 7)
#define ACPI_FADT_TMR_VAL_EXT                   (1 << 8)
#define ACPI_FADT_DCK_CAP                       (1 << 9)
#define ACPI_FADT_RESET_REG_SUP                 (1 << 10)
#define ACPI_FADT_SEALED_CASE                   (1 << 11)
#define ACPI_FADT_HEADLESS                      (1 << 12)
#define ACPI_FADT_CPU_SW_SLP                    (1 << 13)
#define ACPI_FADT_PCI_EXP_WAK                   (1 << 14)
#define ACPI_FADT_USE_PLATFORM_CLOCK            (1 << 15)
#define ACPI_FADT_S4_RTC_STS_VALID              (1 << 16)
#define ACPI_FADT_REMOTE_POWER_ON_CAPABLE       (1 << 17)
#define ACPI_FADT_FORCE_APIC_CLUSTER_MODE       (1 << 18)
#define ACPI_FADT_FORCE_APIC_PHYSDEST_MODE      (1 << 19)
#define ACPI_FADT_HW_REDUCED_ACPI               (1 << 20)
#define ACPI_FADT_LOW_POWER_S0_IDL_CAPABLE      (1 << 21)

#endif /* ACPI_TABLES_FADT_H */
