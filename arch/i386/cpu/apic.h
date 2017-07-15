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

#define SPURIOUS_INTERRUPT 0xFF

extern addr_t lapic_phys_base;
extern addr_t lapic_virt_base;
extern unsigned int ioapics_available;

struct ioapic {
	uint32_t id;
	uint32_t irq_base;
	uint32_t irq_count;
	volatile uint32_t *base;
};

int bsp_apic_init(void);

struct ioapic *apic_add_ioapic(int id, addr_t phys_addr, int irq_base);
struct ioapic *ioapic_from_id(unsigned int id);
struct ioapic *ioapic_from_vector(unsigned int vec);

#endif /* ARCH_I386_APIC_H */
