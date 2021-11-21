/*
 * arch/i386/include/radix/asm/apic.h
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

#ifndef ARCH_I386_RADIX_APIC_H
#define ARCH_I386_RADIX_APIC_H

#include <radix/mm_types.h>
#include <radix/types.h>

extern paddr_t lapic_phys_base;
extern addr_t lapic_virt_base;
extern unsigned int ioapics_available;

enum bus_type {
    BUS_TYPE_ISA,
    BUS_TYPE_EISA,
    BUS_TYPE_PCI,
    BUS_TYPE_UNKNOWN,
    BUS_TYPE_NONE
};

struct ioapic_pin {
    uint8_t irq;
    uint8_t bus_type;
    uint16_t flags;
};

struct ioapic {
    uint32_t id;
    uint32_t irq_base;
    uint32_t irq_count;
    volatile uint32_t *base;
    struct ioapic_pin *pins;
};

#define APIC_LVT_LINT0   0
#define APIC_LVT_LINT1   1
#define APIC_LVT_TIMER   2
#define APIC_LVT_ERROR   3
#define APIC_LVT_PMC     4
#define APIC_LVT_THERMAL 5
#define APIC_LVT_CMCI    6
#define APIC_LVT_MAX     APIC_LVT_CMCI

enum lapic_timer_mode {
    LAPIC_TIMER_ONESHOT,
    LAPIC_TIMER_PERIODIC,
    LAPIC_TIMER_DEADLINE,
    LAPIC_TIMER_UNDEFINED
};

struct lapic_lvt {
    uint8_t vector;
    uint8_t flags;
};

struct lapic {
    uint32_t id;
    uint8_t timer_mode;
    uint8_t timer_div;
    uint16_t lvt_count;
    struct lapic_lvt lvts[APIC_LVT_MAX + 1];
};

#define APIC_ID_ALL 0xFFFFFFFF

/* flags for ioapic_pin and lapic_lvt */
#define APIC_INT_MASKED       (1 << 3)
#define APIC_INT_ACTIVE_HIGH  (1 << 4)
#define APIC_INT_EDGE_TRIGGER (1 << 5)

#define APIC_INT_MODE_FIXED    0x00
#define APIC_INT_MODE_LOW_PRIO 0x01
#define APIC_INT_MODE_SMI      0x02
#define APIC_INT_MODE_NMI      0x04
#define APIC_INT_MODE_INIT     0x05
#define APIC_INT_MODE_STARTUP  0x06
#define APIC_INT_MODE_EXTINT   0x07
#define APIC_INT_MODE_MASK     0x07

int bsp_apic_init(void);

struct ioapic *ioapic_add(int id, addr_t phys_addr, int irq_base);
struct ioapic *ioapic_from_id(unsigned int id);
struct ioapic *ioapic_from_src_irq(unsigned int irq);
struct ioapic *ioapic_from_set_irq(unsigned int irq);

int ioapic_set_nmi(struct ioapic *ioapic, unsigned int pin);
int ioapic_set_smi(struct ioapic *ioapic, unsigned int pin);
int ioapic_set_extint(struct ioapic *ioapic, unsigned int pin);
int ioapic_set_bus(struct ioapic *ioapic, unsigned int pin, int bus_type);
int ioapic_set_irq(struct ioapic *ioapic, unsigned int pin, int irq);
int ioapic_set_polarity(struct ioapic *ioapic, unsigned int pin, int polarity);
int ioapic_set_trigger_mode(struct ioapic *ioapic, unsigned int pin, int trig);
int ioapic_set_delivery_mode(struct ioapic *ioapic, unsigned int pin, int del);

void ioapic_program_pin(struct ioapic *ioapic, unsigned int pin);
void ioapic_program(struct ioapic *ioapic);
int ioapic_mask(struct ioapic *ioapic, unsigned int pin);
int ioapic_unmask(struct ioapic *ioapic, unsigned int pin);

struct lapic *lapic_add(unsigned int id);
struct lapic *lapic_from_id(unsigned int id);

int lapic_set_lvt_mode(uint32_t apic_id, unsigned int pin, uint32_t mode);
int lapic_set_lvt_polarity(uint32_t apic_id, unsigned int pin, int polarity);
int lapic_set_lvt_trigger_mode(uint32_t apic_id, unsigned int pin, int trig);

void lapic_error(void);

int lapic_init(void);
void lapic_timer_calibrate(void);
void lapic_timer_register(void);

int system_smp_capable(void);
void apic_start_smp(unsigned int vector);
void set_ap_active(void);

#endif /* ARCH_I386_RADIX_APIC_H */
