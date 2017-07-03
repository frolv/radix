/*
 * arch/i386/cpu/apic.c
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
#include <acpi/tables/madt.h>

#include <radix/asm/msr.h>
#include <radix/cpu.h>
#include <radix/error.h>
#include <radix/kernel.h>
#include <radix/mm.h>
#include <radix/vmm.h>

#include "apic.h"

#define IA32_APIC_BASE_BSP    (1 << 8)  /* bootstrap processor */
#define IA32_APIC_BASE_EXTD   (1 << 10) /* X2APIC mode enable */
#define IA32_APIC_BASE_ENABLE (1 << 11) /* XAPIC global enable */

static struct acpi_madt *madt;

/* Local APIC base addresses */
static addr_t lapic_phys_base;
static addr_t lapic_virt_base;

static int cpus_available;

/* Mapping between source and global IRQ */
struct irq_map {
	uint16_t bus;
	uint16_t source_irq;
	uint16_t global_irq;
	uint16_t flags;
};
static struct irq_map bus_irqs[16];

#define MAX_IOAPICS 16

struct ioapic {
	uint32_t id;
	uint32_t irq_base;
	uint32_t irq_count;
	volatile uint32_t *base;
};
static struct ioapic ioapic_list[MAX_IOAPICS];
static unsigned int ioapics_available;

#define IOAPIC_IOREGSEL 0
#define IOAPIC_IOREGWIN 4

#define IOAPIC_REG_ID   0
#define IOAPIC_REG_VER  1
#define IOAPIC_REG_ARB  2

DEFINE_PER_CPU(int, apic_id);

static uint32_t ioapic_reg_read(struct ioapic *ioapic, int reg)
{
	ioapic->base[IOAPIC_IOREGSEL] = reg;
	return ioapic->base[IOAPIC_IOREGWIN];
}

static void ioapic_reg_write(struct ioapic *ioapic, int reg, uint32_t value)
{
	ioapic->base[IOAPIC_IOREGSEL] = reg;
	ioapic->base[IOAPIC_IOREGWIN] = value;
}

static struct ioapic *ioapic_from_id(unsigned int id)
{
	size_t i;

	for (i = 0; i < ioapics_available; ++i) {
		if (ioapic_list[i].id == id)
			return ioapic_list + i;
	}

	return NULL;
}

/*
 * ioapic_from_vector:
 * Return the I/O APIC that controls the given interrupt vector.
 */
static struct ioapic *ioapic_from_vector(unsigned int vec)
{
	size_t i;

	for (i = 0; i < ioapics_available; ++i) {
		if (vec >= ioapic_list[i].irq_base &&
		    vec < ioapic_list[i].irq_base + ioapic_list[i].irq_count)
			return ioapic_list + i;
	}

	return NULL;
}

static void apic_enable(addr_t base)
{
	wrmsr(IA32_APIC_BASE, (base & PAGE_MASK) | IA32_APIC_BASE_ENABLE, 0);
}

static uint32_t apic_reg_read(uint16_t reg)
{
	return *(uint32_t *)(lapic_virt_base + reg);
}

static void apic_reg_write(uint16_t reg, uint32_t value)
{
	*(uint32_t *)(lapic_virt_base + reg) = value;
}

/*
 * read_apic_id:
 * Read the local APIC ID of the executing processor
 * and save it in the `apic_id` per-CPU variable.
 */
void read_apic_id(void)
{
	uint32_t eax, edx;
	int id;

	if (!cpu_supports(CPUID_APIC)) {
		this_cpu_write(apic_id, -1);
		return;
	}

	if (cpu_supports(CPUID_X2APIC)) {
		/* check if operating in X2APIC mode */
		rdmsr(IA32_APIC_BASE, &eax, &edx);
		if (eax & IA32_APIC_BASE_EXTD) {
			rdmsr(IA32_X2APIC_APICID, &eax, &edx);
			this_cpu_write(apic_id, eax);
			return;
		}
	}

	id = apic_reg_read(0x20) >> 24;
	this_cpu_write(apic_id, id);
}

/*
 * apic_init:
 * Configure the LAPIC to send interrupts and enable it.
 */
void apic_init(void)
{
	lapic_virt_base = (addr_t)vmalloc(PAGE_SIZE);
	map_page_kernel(lapic_virt_base, lapic_phys_base,
	                PROT_WRITE, PAGE_CP_UNCACHEABLE);
	apic_enable(lapic_phys_base);

	/* Enable APIC and set spurious interrupt vector */
	apic_reg_write(0xF0, 0x100 | SPURIOUS_INTERRUPT);
}

static void apic_parse_lapic(struct acpi_madt_local_apic *s)
{
	cpus_available += s->flags & 1;
}

static void apic_parse_ioapic(struct acpi_madt_io_apic *s)
{
	struct ioapic *ioapic;
	uint32_t irq_count;
	addr_t base;

	if (ioapics_available == MAX_IOAPICS)
		return;

	base = (addr_t)vmalloc(PAGE_SIZE);
	map_page_kernel(base, s->address, PROT_WRITE, PAGE_CP_UNCACHEABLE);

	ioapic = &ioapic_list[ioapics_available++];
	ioapic->id = s->id;
	ioapic->irq_base = s->global_irq_base;
	ioapic->base = (uint32_t *)base;

	irq_count = ioapic_reg_read(ioapic, IOAPIC_REG_VER);
	irq_count = ((irq_count >> 16) & 0xFF) + 1;
	ioapic->irq_count = irq_count;
}

static void apic_parse_override(struct acpi_madt_interrupt_override *s)
{
	size_t i;

	/* Set bus for all ISA IRQs */
	if (bus_irqs[0].bus == 0xFFFF) {
		for (i = 0; i < 16; ++i)
			bus_irqs[i].bus = s->bus_source;
	}

	bus_irqs[s->irq_source].bus = s->bus_source;
	bus_irqs[s->irq_source].global_irq = s->global_irq;
	bus_irqs[s->irq_source].flags = s->flags;
}

/*
 * apic_parse_madt:
 * Check that the MADT ACPI table exists and is valid, and store pointer to it.
 */
int apic_parse_madt(void)
{
	struct acpi_subtable_header *header;
	unsigned char *p, *end;
	size_t i;

	for (i = 0; i < 16; ++i) {
		bus_irqs[i].bus = 0xFFFF;
		bus_irqs[i].source_irq = i;
		bus_irqs[i].global_irq = i;
		bus_irqs[i].flags = ACPI_MADT_INTI_POLARITY_ACTIVE_HIGH |
			ACPI_MADT_INTI_TRIGGER_MODE_EDGE;
	}

	madt = acpi_find_table(ACPI_MADT_SIGNATURE);
	if (!madt)
		return 1;

	lapic_phys_base = madt->address;

	p = (unsigned char *)(madt + 1);
	end = (unsigned char *)madt + madt->header.length;

	while (p < end) {
		header = (struct acpi_subtable_header *)p;
		switch (header->type) {
		case ACPI_MADT_LOCAL_APIC:
			apic_parse_lapic((struct acpi_madt_local_apic *)header);
			break;
		case ACPI_MADT_IO_APIC:
			apic_parse_ioapic((struct acpi_madt_io_apic *)header);
			break;
		case ACPI_MADT_INTERRUPT_OVERRIDE:
			apic_parse_override((struct acpi_madt_interrupt_override *)header);
			break;
		default:
			break;
		}
		p += header->length;
	}

	return 0;
}
