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

#include <radix/asm/mps.h>
#include <radix/asm/msr.h>
#include <radix/asm/bios.h>

#include <radix/cpu.h>
#include <radix/error.h>
#include <radix/kernel.h>
#include <radix/mm.h>
#include <radix/vmm.h>

#include <rlibc/string.h>

#include "apic.h"
#include "isr.h"

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
static struct irq_map isa_irqs[ISA_IRQ_COUNT];
static uint16_t isa_irq_bus = 0xFFFF;

#ifdef CONFIG_MAX_IOAPICS
#define MAX_IOAPICS CONFIG_MAX_IOAPICS
#else
#define MAX_IOAPICS 8
#endif

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
static void read_apic_id(void)
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
	read_apic_id();
	apic_enable(lapic_phys_base);
	apic_reg_write(0xF0, 0x100 | SPURIOUS_INTERRUPT);
}

static void apic_add_lapic(int valid_cpu)
{
	cpus_available += valid_cpu;
}

/* mp table i/o apic entries don't store irq base, so we need to keep track */
static int curr_ioapic_irq_base = 0;

static void apic_add_ioapic(int id, addr_t phys_addr, int irq_base)
{
	struct ioapic *ioapic;
	uint32_t irq_count;
	addr_t base;

	if (ioapics_available == MAX_IOAPICS)
		return;

	base = (addr_t)vmalloc(PAGE_SIZE);
	map_page_kernel(base, phys_addr, PROT_WRITE, PAGE_CP_UNCACHEABLE);

	ioapic = &ioapic_list[ioapics_available++];
	ioapic->id = id;
	ioapic->irq_base = irq_base;
	ioapic->base = (uint32_t *)base;

	irq_count = ioapic_reg_read(ioapic, IOAPIC_REG_VER);
	irq_count = ((irq_count >> 16) & 0xFF) + 1;
	ioapic->irq_count = irq_count;

	curr_ioapic_irq_base += irq_count;
}

static void apic_add_override(int bus, int src, int irq, unsigned int flags)
{
	size_t i;

	/* set bus for all ISA IRQs when first called */
	if (isa_irq_bus == 0xFFFF) {
		for (i = 0; i < ISA_IRQ_COUNT; ++i)
			isa_irqs[i].bus = bus;
		isa_irq_bus = bus;
	}

	isa_irqs[src].bus = bus;
	isa_irqs[src].global_irq = irq;
	isa_irqs[src].flags = flags;
}

static void __madt_lapic(struct acpi_madt_local_apic *s)
{
	apic_add_lapic(!!(s->flags & 1));
}

static void __madt_ioapic(struct acpi_madt_io_apic *s)
{
	apic_add_ioapic(s->id, s->address, s->global_irq_base);
}

static void __madt_override(struct acpi_madt_interrupt_override *s)
{
	apic_add_override(s->bus_source, s->irq_source,
                          s->global_irq, s->flags);
}

/*
 * apic_parse_madt:
 * Parse the ACPI MADT table and extract APIC information.
 */
int apic_parse_madt(void)
{
	struct acpi_subtable_header *header;
	unsigned char *p, *end;

	madt = acpi_find_table(ACPI_MADT_SIGNATURE);
	if (!madt)
		return 1;

	lapic_phys_base = madt->address;

	p = (unsigned char *)(madt + 1);
	end = (unsigned char *)madt + madt->header.length;

	while (p < end) {
		header = (struct acpi_subtable_header *)p;
		/* TODO: add other MADT entries */
		switch (header->type) {
		case ACPI_MADT_LOCAL_APIC:
			__madt_lapic((struct acpi_madt_local_apic *)header);
			break;
		case ACPI_MADT_IO_APIC:
			__madt_ioapic((struct acpi_madt_io_apic *)header);
			break;
		case ACPI_MADT_INTERRUPT_OVERRIDE:
			__madt_override((struct acpi_madt_interrupt_override *)
			                header);
			break;
		default:
			break;
		}
		p += header->length;
	}

	return 0;
}

static int byte_sum(void *start, size_t len)
{
	char *s;
	int sum;

	for (sum = 0, s = start; s < (char *)start + len; ++s)
	     sum += *s;

	return sum & 0xFF;
}

/*
 * find_mp_config_table
 * Find the location of the MP spec configuration table and verify
 * its vailidity. Return a pointer to the table if successful.
 */
struct mp_config_table *find_mp_config_table(void)
{
	struct mp_floating_pointer *fp;
	struct mp_config_table *mp;

	fp = bios_find_signature(MP_FP_SIGNATURE, sizeof fp->signature, 16);
	if (!fp)
		return NULL;

	if (byte_sum(fp, fp->length << 4) != 0)
		return NULL;

	mp = (struct mp_config_table *)phys_to_virt(fp->config_base);
	if (memcmp(mp->signature, MP_CONFIG_SIGNATURE, sizeof mp->signature))
		return NULL;

	return byte_sum(mp, mp->length) == 0 ? mp : NULL;
}

static void __mp_processor(struct mp_table_processor *s)
{
	apic_add_lapic(!!(s->cpu_flags & MP_PROCESSOR_ACTIVE));
}

static void __mp_bus(struct mp_table_bus *s)
{
	size_t i;

	if (memcmp(s->bus_type, MP_BUS_SIGNATURE_ISA, 6) == 0) {
		isa_irq_bus = s->bus_id;
		for (i = 0; i < ISA_IRQ_COUNT; ++i)
			isa_irqs[i].bus = s->bus_id;
	}
}

static void __mp_ioapic(struct mp_table_io_apic *s)
{
	apic_add_ioapic(s->ioapic_id, s->ioapic_base, curr_ioapic_irq_base);
}

static void __mp_io_interrupt(struct mp_table_io_interrupt *s)
{
	/* TODO */
	(void)s;
}

static void __mp_local_int(struct mp_table_local_interrupt *s)
{
	/* TODO */
	(void)s;
}

int apic_parse_mp_tables(void)
{
	struct mp_config_table *mp;
	uint8_t *s;
	size_t i;

	mp = find_mp_config_table();
	if (!mp)
		return 1;

	s = (uint8_t *)(mp + 1);
	for (i = 0; i < mp->entry_count; ++i) {
		switch (*s) {
		case MP_TABLE_PROCESSOR:
			__mp_processor((struct mp_table_processor *)s);
			s += 20;
			continue;
		case MP_TABLE_BUS:
			__mp_bus((struct mp_table_bus *)s);
			break;
		case MP_TABLE_IO_APIC:
			__mp_ioapic((struct mp_table_io_apic *)s);
			break;
		case MP_TABLE_IO_INTERRUPT:
			__mp_io_interrupt((struct mp_table_io_interrupt *)s);
			break;
		case MP_TABLE_LOCAL_INTERRUPT:
			__mp_local_int((struct mp_table_local_interrupt *)s);
			break;
		}
		s += 8;
	}

	return 0;
}

int bsp_apic_init(void)
{
	size_t i;

	for (i = 0; i < ISA_IRQ_COUNT; ++i) {
		isa_irqs[i].bus = 0xFFFF;
		isa_irqs[i].source_irq = i;
		isa_irqs[i].global_irq = i;
		isa_irqs[i].flags = ACPI_MADT_INTI_POLARITY_ACTIVE_HIGH |
			ACPI_MADT_INTI_TRIGGER_MODE_EDGE;
	}

	if (apic_parse_madt() != 0 && apic_parse_mp_tables() != 0)
		return 1;

	lapic_virt_base = (addr_t)vmalloc(PAGE_SIZE);
	map_page_kernel(lapic_virt_base, lapic_phys_base,
	                PROT_WRITE, PAGE_CP_UNCACHEABLE);
	apic_init();

	return 0;
}
