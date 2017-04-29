/*
 * include/acpi/tables/madt.h
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

#ifndef ACPI_TABLES_MADT_H
#define ACPI_TABLES_MADT_H

#include <acpi/tables/sdt.h>

enum {
	ACPI_MADT_LOCAL_APIC            = 0,
	ACPI_MADT_IO_APIC               = 1,
	ACPI_MADT_INTERRUPT_OVERRIDE    = 2,
	ACPI_MADT_NMI_SOURCE            = 3,
	ACPI_MADT_LOCAL_APIC_NMI        = 4,
	ACPI_MADT_LOCAL_APIC_OVERRIDE   = 5,
	ACPI_MADT_IO_SAPIC              = 6,
	ACPI_MADT_LOCAL_SAPIC           = 7,
	ACPI_MADT_INTERRUPT_SOURCE      = 8,
	ACPI_MADT_LOCAL_X2APIC          = 9,
	ACPI_MADT_LOCAL_X2APIC_NMI      = 10,
	ACPI_MADT_GENERIC_INTERRUPT     = 11,
	ACPI_MADT_GENERIC_DISTRIBUTOR   = 12,
	ACPI_MADT_GENERIC_MSI_FRAME     = 13,
	ACPI_MADT_GENERIC_REDISTRIBUTOR = 14,
	ACPI_MADT_GENERIC_TRANSLATOR    = 15
};

#define ACPI_MADT_SIGNATURE "APIC"

struct acpi_madt {
	struct acpi_sdt_header header;
	uint32_t address;
	uint32_t flags;
};

/*
 * Corresponding subtables for above types.
 * From Linux include/acpi/actbl1.h
 */

struct acpi_madt_local_apic {
	struct acpi_subtable_header header;
	uint8_t processor_id;
	uint8_t apic_id;
	uint32_t flags;
};

struct acpi_madt_io_apic {
	struct acpi_subtable_header header;
	uint8_t id;
	uint8_t reserved;
	uint32_t address;
	uint32_t global_irq_base;
};

struct acpi_madt_interrupt_override {
	struct acpi_subtable_header header;
	uint8_t bus_source;
	uint8_t irq_source;
	uint32_t global_irq;
	uint16_t flags;
};

/* flags for the above field */
#define ACPI_MADT_INTI_POLARITY_CONFORMS        0
#define ACPI_MADT_INTI_POLARITY_ACTIVE_HIGH     1
#define ACPI_MADT_INTI_POLARITY_ACTIVE_RESERVED 2
#define ACPI_MADT_INTI_POLARITY_ACTIVE_LOW      3

#define ACPI_MADT_INTI_TRIGGER_MODE_CONFORMS    0
#define ACPI_MADT_INTI_TRIGGER_MODE_EDGE        (1 << 2)
#define ACPI_MADT_INTI_TRIGGER_MODE_RESERVED    (2 << 2)
#define ACPI_MADT_INTI_TRIGGER_MODE_LEVEL       (3 << 2)


struct acpi_madt_nmi_source {
	struct acpi_subtable_header header;
	uint16_t flags;
	uint32_t global_irq;
};

struct acpi_madt_local_apic_nmi {
	struct acpi_subtable_header header;
	uint8_t processor_id;
	uint16_t flags;
	uint8_t lint;
};

struct acpi_madt_local_apic_override {
	struct acpi_subtable_header header;
	uint16_t reserved;
	uint64_t address;
};

struct acpi_madt_io_sapic {
	struct acpi_subtable_header header;
	uint8_t id;
	uint8_t reserved;
	uint32_t global_irq_base;
	uint64_t address;
};

struct acpi_madt_local_sapic {
	struct acpi_subtable_header header;
	uint8_t processor_id;
	uint8_t sapic_id;
	uint8_t sapic_eid;
	uint8_t reserved[3];
	uint32_t lapic_flags;
	uint32_t uid;
	char uid_string[1];
};

struct acpi_madt_interrupt_source {
	struct acpi_subtable_header header;
	uint16_t inti_flags;
	uint8_t type;
	uint8_t id;
	uint8_t eid;
	uint8_t io_sapic_vector;
	uint32_t global_irq;
	uint32_t flags;
};

struct acpi_madt_local_x2apic {
	struct acpi_subtable_header header;
	uint16_t reserved;
	uint32_t local_apic_id;
	uint32_t lapic_flags;
	uint32_t uid;
};

struct acpi_madt_local_x2apic_nmi {
	struct acpi_subtable_header header;
	uint16_t flags;
	uint32_t uid;
	uint8_t lint;
	uint8_t reserved[3];
};

struct acpi_madt_generic_interrupt {
	struct acpi_subtable_header header;
	uint16_t reserved;
	uint32_t cpu_interace_number;
	uint32_t uid;
	uint32_t flags;
	uint32_t parking_version;
	uint32_t performance_interrupt;
	uint64_t parked_address;
	uint64_t base_address;
	uint64_t gicv_base_address;
	uint64_t gich_base_address;
	uint32_t vgic_interrupt;
	uint64_t gicr_base_address;
	uint64_t arm_mpidr;
	uint8_t efficiency_class;
	uint8_t reserved2[3];
};

struct acpi_madt_generic_distributor {
	struct acpi_subtable_header header;
	uint16_t reserved;
	uint32_t gic_id;
	uint64_t base_address;
	uint32_t global_irq_base;
	uint8_t version;
	uint8_t reserved2[3];
};

struct acpi_madt_generic_msi_frame {
	struct acpi_subtable_header header;
	uint16_t reserved;
	uint32_t msi_frame_id;
	uint64_t base_address;
	uint32_t flags;
	uint16_t spi_count;
	uint16_t spi_base;
};

struct acpi_madt_generic_redistributor {
	struct acpi_subtable_header header;
	uint32_t reserved;
	uint64_t base_address;
	uint32_t length;
};

struct acpi_madt_generic_translator {
	struct acpi_subtable_header header;
	uint16_t reserved;
	uint32_t translation_id;
	uint64_t base_address;
	uint32_t reserved2;
};

#endif /* ACPI_TABLES_MADT_H */
