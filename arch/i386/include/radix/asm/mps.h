/*
 * arch/i386/include/radix/asm/mps.h
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

#ifndef ARCH_I386_RADIX_MPS_H
#define ARCH_I386_RADIX_MPS_H

/* x86 multiprocessor specification */

#define MP_FP_SIGNATURE     "_MP_"
#define MP_CONFIG_SIGNATURE "PCMP"

#include <radix/types.h>

struct mp_floating_pointer {
    char signature[4];
    uint32_t config_base;
    uint8_t length;
    uint8_t revision;
    uint8_t checksum;
    uint8_t default_configuration;
    uint32_t features;
};

struct mp_config_table {
    char signature[4];
    uint16_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[8];
    char product_id[12];
    uint32_t oem_table;
    uint16_t oem_table_size;
    uint16_t entry_count;
    uint32_t lapic_base;
    uint16_t extd_length;
    uint8_t extd_checksum;
    uint8_t reserved;
};

enum {
    MP_TABLE_PROCESSOR,
    MP_TABLE_BUS,
    MP_TABLE_IO_APIC,
    MP_TABLE_IO_INTERRUPT,
    MP_TABLE_LOCAL_INTERRUPT
};

struct mp_table_processor {
    uint8_t type;
    uint8_t apic_id;
    uint8_t apic_version;
    uint8_t cpu_flags;
    uint32_t signature;
    uint32_t features;
    uint64_t reserved;
};

#define MP_PROCESSOR_ACTIVE (1 << 0)
#define MP_PROCESSOR_BSP    (1 << 1)

struct mp_table_bus {
    uint8_t type;
    uint8_t bus_id;
    char bus_type[6];
};

struct mp_table_io_apic {
    uint8_t type;
    uint8_t ioapic_id;
    uint8_t ioapic_version;
    uint8_t flags;
    uint32_t ioapic_base;
};

#define MP_IO_APIC_ACTIVE (1 << 0)

struct mp_table_io_interrupt {
    uint8_t type;
    uint8_t interrupt_type;
    uint16_t flags;
    uint8_t source_bus;
    uint8_t source_irq;
    uint8_t dest_ioapic;
    uint8_t dest_intin;
};

struct mp_table_local_interrupt {
    uint8_t type;
    uint8_t interrupt_type;
    uint16_t flags;
    uint8_t source_bus;
    uint8_t source_irq;
    uint8_t dest_lapic;
    uint8_t dest_lintin;
};

#define MP_INTERRUPT_TYPE_INT    0
#define MP_INTERRUPT_TYPE_NMI    1
#define MP_INTERRUPT_TYPE_SMI    2
#define MP_INTERRUPT_TYPE_EXTINT 3

#define MP_INTERRUPT_POLARITY_CONFORMS    0
#define MP_INTERRUPT_POLARITY_ACTIVE_HIGH 1
#define MP_INTERRUPT_POLARITY_RESERVED    2
#define MP_INTERRUPT_POLARITY_ACTIVE_LOW  3
#define MP_INTERRUPT_POLARITY_MASK        3

#define MP_INTERRUPT_TRIGGER_MODE_CONFORMS 0
#define MP_INTERRUPT_TRIGGER_MODE_EDGE     (1 << 2)
#define MP_INTERRUPT_TRIGGER_MODE_RESERVED (2 << 2)
#define MP_INTERRUPT_TRIGGER_MODE_LEVEL    (3 << 2)
#define MP_INTERRUPT_TRIGGER_MODE_MASK     (3 << 2)

#define MP_BUS_SIGNATURE_CBUS        "CBUS  "
#define MP_BUS_SIGNATURE_CBUS_II     "CBUSII"
#define MP_BUS_SIGNATURE_EISA        "EISA  "
#define MP_BUS_SIGNATURE_FUTURE      "FUTURE"
#define MP_BUS_SIGNATURE_INTERNAL    "INTERN"
#define MP_BUS_SIGNATURE_ISA         "ISA   "
#define MP_BUS_SIGNATURE_MULTIBUS    "MBI   "
#define MP_BUS_SIGNATURE_MULTIBUS_II "MBII  "
#define MP_BUS_SIGNATURE_MCA         "MCA   "
#define MP_BUS_SIGNATURE_MPI         "MPI   "
#define MP_BUS_SIGNATURE_MPSA        "MPSA  "
#define MP_BUS_SIGNATURE_NUBUS       "NUBUS "
#define MP_BUS_SIGNATURE_PCI         "PCI   "
#define MP_BUS_SIGNATURE_PCMCIA      "PCMCIA"
#define MP_BUS_SIGNATURE_TC          "TC    "
#define MP_BUS_SIGNATURE_VL          "VL    "
#define MP_BUS_SIGNATURE_VME         "VME   "
#define MP_BUS_SIGNATURE_XPRESS      "XPRESS"

int parse_mp_tables(void);

#endif /* ARCH_I386_RADIX_MPS_H */
