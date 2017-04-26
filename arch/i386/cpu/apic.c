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
#include <radix/page.h>

#include "apic.h"

#define IA32_APIC_BASE_BSP    (1 << 8)  /* bootstrap processor */
#define IA32_APIC_BASE_EXTD   (1 << 10) /* X2APIC mode enable */
#define IA32_APIC_BASE_ENABLE (1 << 11) /* XAPIC global enable */

static struct acpi_madt *madt;

static addr_t lapic_base; /* Local APIC base address */
static int cpus_available;

static void apic_parse_lapic(struct acpi_madt_local_apic *s)
{
	printf("ACPI: LAPIC (processor_id %u lapic_id %u %s)\n",
	       s->processor_id, s->apic_id,
	       s->flags & 1 ? "enabled" : "disabled");

	cpus_available += s->flags & 1;
}

static void apic_parse_ioapic(struct acpi_madt_io_apic *s)
{
	printf("ACPI: IOAPIC (id %u address 0x%08lX irq_base %u)\n",
	       s->id, s->address, s->global_irq_base);
}

static void apic_parse_override(struct acpi_madt_interrupt_override *s)
{
	printf("ACPI: OVERRIDE (bus %u source_irq %u global_irq %u)\n",
	       s->bus_source, s->irq_source, s->global_irq);
}

/*
 * apic_parse_madt:
 * Check that the MADT ACPI table exists and is valid, and store pointer to it.
 */
int apic_parse_madt(void)
{
	struct acpi_subtable_header *header;
	unsigned char *p, *end;

	madt = acpi_find_table(ACPI_MADT_SIGNATURE);
	if (!madt)
		return 1;

	lapic_base = madt->address;

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

static void apic_enable(addr_t base)
{
	wrmsr(IA32_APIC_BASE, (base & PAGE_MASK) | IA32_APIC_BASE_ENABLE, 0);
}

static uint32_t apic_reg_read(uint16_t reg)
{
	return *(uint32_t *)(__ARCH_APIC_VIRT_PAGE + reg);
}

static void apic_reg_write(uint16_t reg, int32_t value)
{
	*(uint32_t *)(__ARCH_APIC_VIRT_PAGE + reg) = value;
}

/*
 * processor_id:
 * Return the local APIC ID of the executing processor.
 */
uint32_t processor_id(void)
{
	uint32_t eax, edx;

	if (!cpu_supports(CPUID_APIC))
		return 0;

	if (cpu_supports(CPUID_X2APIC)) {
		/* check if operating in X2APIC mode */
		rdmsr(IA32_APIC_BASE, &eax, &edx);
		if (eax & IA32_APIC_BASE_EXTD) {
			rdmsr(IA32_X2APIC_APICID, &eax, &edx);
			return eax;
		}
	}

	return apic_reg_read(0x20) >> 24;
}

/*
 * apic_init:
 * Configure the LAPIC to send interrupts and enable it.
 */
void apic_init(void)
{
	map_page(__ARCH_APIC_VIRT_PAGE, lapic_base);
	apic_enable(lapic_base);

	/* Enable APIC and set spurious interrupt vector */
	apic_reg_write(0xF0, 0x100 | SPURIOUS_INTERRUPT);
}
