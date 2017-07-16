/*
 * arch/i386/cpu/apic.h
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

#ifndef ARCH_I386_APIC_H
#define ARCH_I386_APIC_H

#include <radix/mm_types.h>

#define IRQ_NMI      0xE0
#define IRQ_SMI      0xE1
#define IRQ_EXTINT   0xE2
#define IRQ_SPURIOUS 0xFF

extern addr_t lapic_phys_base;
extern addr_t lapic_virt_base;
extern unsigned int ioapics_available;

struct ioapic_pin;

struct ioapic {
	uint32_t id;
	uint32_t irq_base;
	uint32_t irq_count;
	volatile uint32_t *base;
	struct ioapic_pin *pins;
};

enum bus_type {
	BUS_TYPE_ISA,
	BUS_TYPE_EISA,
	BUS_TYPE_PCI,
	BUS_TYPE_UNKNOWN,
	BUS_TYPE_NONE
};

int bsp_apic_init(void);

struct ioapic *ioapic_add(int id, addr_t phys_addr, int irq_base);
struct ioapic *ioapic_from_id(unsigned int id);
struct ioapic *ioapic_from_vector(unsigned int vec);

int ioapic_set_nmi(struct ioapic *ioapic, unsigned int pin);
int ioapic_set_smi(struct ioapic *ioapic, unsigned int pin);
int ioapic_set_extint(struct ioapic *ioapic, unsigned int pin);
int ioapic_set_bus(struct ioapic *ioapic, unsigned int pin, int bus_type);

#endif /* ARCH_I386_APIC_H */
