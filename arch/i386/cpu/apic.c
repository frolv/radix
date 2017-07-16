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

#include <acpi/tables/madt.h>

#include <radix/asm/acpi.h>
#include <radix/asm/apic.h>
#include <radix/asm/mps.h>
#include <radix/asm/msr.h>

#include <radix/cpu.h>
#include <radix/cpumask.h>
#include <radix/irq.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/mm.h>
#include <radix/percpu.h>
#include <radix/slab.h>
#include <radix/vmm.h>

#include <rlibc/string.h>

#include "isr.h"

#ifdef CONFIG_MAX_IOAPICS
#define MAX_IOAPICS CONFIG_MAX_IOAPICS
#else
#define MAX_IOAPICS 8
#endif

static struct ioapic ioapic_list[MAX_IOAPICS];
unsigned int ioapics_available = 0;

#define IOAPIC_IOREGSEL 0
#define IOAPIC_IOREGWIN 4

#define IOAPIC_REG_ID   0
#define IOAPIC_REG_VER  1
#define IOAPIC_REG_ARB  2


#define IA32_APIC_BASE_BSP    (1 << 8)  /* bootstrap processor */
#define IA32_APIC_BASE_EXTD   (1 << 10) /* X2APIC mode enable */
#define IA32_APIC_BASE_ENABLE (1 << 11) /* XAPIC global enable */

/* Local APIC base addresses */
addr_t lapic_phys_base;
addr_t lapic_virt_base;

#define __ET    APIC_INT_EDGE_TRIGGER
#define __AH    APIC_INT_ACTIVE_HIGH
#define __MASK  APIC_INT_MASKED

static struct lapic_lvt lapic_lvt_default[] = {
	/* LINT0: EXTINT */
	{ 0, APIC_LVT_MODE_EXTINT | __ET | __AH | __MASK },
	/* LINT1: NMI */
	{ 0, APIC_LVT_MODE_NMI | __ET | __AH },
	/* timer */
	{ APIC_IRQ_TIMER, APIC_LVT_MODE_FIXED | __ET | __AH | __MASK },
	/* error */
	{ APIC_IRQ_ERROR, APIC_LVT_MODE_FIXED | __ET | __AH },
	/* PMC */
	{ 0, APIC_LVT_MODE_NMI | __ET | __AH | __MASK },
	/* thermal */
	{ APIC_IRQ_THERMAL, APIC_LVT_MODE_FIXED | __ET | __AH | __MASK },
	/* CMCI */
	{ APIC_IRQ_CMCI, APIC_LVT_MODE_FIXED | __ET | __AH | __MASK }
};

static struct lapic lapic_list[MAX_CPUS];
static unsigned int cpus_available = 0;

DEFINE_PER_CPU(struct lapic *, local_apic);


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

struct ioapic *ioapic_from_id(unsigned int id)
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
struct ioapic *ioapic_from_vector(unsigned int vec)
{
	size_t i;

	for (i = 0; i < ioapics_available; ++i) {
		if (vec >= ioapic_list[i].irq_base &&
		    vec < ioapic_list[i].irq_base + ioapic_list[i].irq_count)
			return ioapic_list + i;
	}

	return NULL;
}

struct ioapic *ioapic_add(int id, addr_t phys_addr, int irq_base)
{
	struct ioapic *ioapic;
	uint32_t irq_count;
	addr_t base;
	size_t i;

	if (ioapics_available == MAX_IOAPICS)
		return NULL;

	base = (addr_t)vmalloc(PAGE_SIZE);
	map_page_kernel(base, phys_addr, PROT_WRITE, PAGE_CP_UNCACHEABLE);

	ioapic = &ioapic_list[ioapics_available++];
	ioapic->id = id;
	ioapic->irq_base = irq_base;
	ioapic->base = (uint32_t *)base;

	irq_count = ioapic_reg_read(ioapic, IOAPIC_REG_VER);
	irq_count = ((irq_count >> 16) & 0xFF) + 1;
	ioapic->irq_count = irq_count;

	ioapic->pins = kmalloc(irq_count * sizeof *ioapic->pins);
	if (!ioapic->pins)
		panic("failed to allocate memory for I/O APIC %d\n", id);

	for (i = 0; i < irq_count; ++i) {
		ioapic->pins[i].irq = irq_base + i;

		/*
		 * Assume that IRQ 0 is an EXTINT, 1-15 are ISA IRQs
		 * and the rest are PCI.
		 */
		if (ioapic->pins[i].irq == 0) {
			ioapic_set_extint(ioapic, i);
		} else if (ioapic->pins[i].irq < ISA_IRQ_COUNT) {
			ioapic->pins[i].bus_type = BUS_TYPE_ISA;
			ioapic->pins[i].flags = APIC_INT_ACTIVE_HIGH |
				APIC_INT_EDGE_TRIGGER | APIC_INT_MASKED;
		} else {
			ioapic->pins[i].bus_type = BUS_TYPE_PCI;
			ioapic->pins[i].flags = APIC_INT_MASKED;
		}
	}

	return ioapic;
}

static int __ioapic_set_special(struct ioapic *ioapic,
                                unsigned int pin,
                                unsigned int irq)
{
	if (pin >= ioapic->irq_count)
		return EINVAL;

	ioapic->pins[pin].bus_type = BUS_TYPE_UNKNOWN;
	ioapic->pins[pin].irq = irq;
	ioapic->pins[pin].flags &= ~APIC_INT_MASKED;
	ioapic->pins[pin].flags |= APIC_INT_ACTIVE_HIGH | APIC_INT_EDGE_TRIGGER;

	return 0;
}

int ioapic_set_nmi(struct ioapic *ioapic, unsigned int pin)
{
	return __ioapic_set_special(ioapic, pin, APIC_IRQ_NMI);
}

int ioapic_set_smi(struct ioapic *ioapic, unsigned int pin)
{
	return __ioapic_set_special(ioapic, pin, APIC_IRQ_SMI);
}

int ioapic_set_extint(struct ioapic *ioapic, unsigned int pin)
{
	return __ioapic_set_special(ioapic, pin, APIC_IRQ_EXTINT);
}

int ioapic_set_bus(struct ioapic *ioapic, unsigned int pin, int bus_type)
{
	if (pin >= ioapic->irq_count)
		return EINVAL;

	ioapic->pins[pin].bus_type = bus_type;
	return 0;
}

int ioapic_set_vector(struct ioapic *ioapic, unsigned int pin, int vec)
{
	if (pin >= ioapic->irq_count || vec > NUM_ISR_VECTORS - IRQ_BASE)
		return EINVAL;

	ioapic->pins[pin].irq = vec;
	return 0;
}

int ioapic_set_polarity(struct ioapic *ioapic, unsigned int pin, int polarity)
{
	if (pin >= ioapic->irq_count)
		return EINVAL;

	if (polarity == MP_INTERRUPT_POLARITY_ACTIVE_HIGH ||
	    polarity == ACPI_MADT_INTI_POLARITY_ACTIVE_HIGH)
		ioapic->pins[pin].flags |= APIC_INT_ACTIVE_HIGH;
	else if (polarity == MP_INTERRUPT_POLARITY_ACTIVE_LOW ||
		 polarity == ACPI_MADT_INTI_POLARITY_ACTIVE_LOW)
		ioapic->pins[pin].flags &= ~APIC_INT_ACTIVE_HIGH;
	else
		return EINVAL;

	return 0;
}

int ioapic_set_trigger_mode(struct ioapic *ioapic, unsigned int pin, int trig)
{
	if (pin >= ioapic->irq_count)
		return EINVAL;

	if (trig == MP_INTERRUPT_TRIGGER_MODE_EDGE ||
	    trig == ACPI_MADT_INTI_TRIGGER_MODE_EDGE)
		ioapic->pins[pin].flags |= APIC_INT_EDGE_TRIGGER;
	else if (trig == MP_INTERRUPT_TRIGGER_MODE_LEVEL ||
		 trig == ACPI_MADT_INTI_TRIGGER_MODE_LEVEL)
		ioapic->pins[pin].flags &= ~APIC_INT_EDGE_TRIGGER;
	else
		return EINVAL;

	return 0;
}

static void lapic_enable(addr_t base)
{
	wrmsr(IA32_APIC_BASE, (base & PAGE_MASK) | IA32_APIC_BASE_ENABLE, 0);
}

static uint32_t lapic_reg_read(uint16_t reg)
{
	return *(uint32_t *)(lapic_virt_base + reg);
}

static void lapic_reg_write(uint16_t reg, uint32_t value)
{
	*(uint32_t *)(lapic_virt_base + reg) = value;
}

struct lapic *lapic_from_id(unsigned int id)
{
	size_t i;

	for (i = 0; i < cpus_available; ++i) {
		if (lapic_list[i].id == id)
			return lapic_list + i;
	}

	return NULL;
}

struct lapic *lapic_add(unsigned int id)
{
	struct lapic *lapic;

	if (cpus_available == MAX_CPUS)
		return NULL;

	lapic = &lapic_list[cpus_available++];
	lapic->id = id;
	lapic->timer_mode = LAPIC_TIMER_UNDEFINED;
	lapic->timer_div = 1;
	memcpy(&lapic->lvts, &lapic_lvt_default, sizeof lapic->lvts);

	return lapic;
}

static void __lapic_set_lvt_mode(struct lapic *lapic, int pin, uint32_t mode)
{
	lapic->lvts[pin].flags &= ~APIC_LVT_MODE_MASK;
	lapic->lvts[pin].flags |= mode;
}

int lapic_set_lvt_mode(uint32_t apic_id, unsigned int pin, uint32_t mode)
{
	struct lapic *lapic;
	size_t i;

	if (pin > APIC_LVT_MAX)
		return EINVAL;

	if (apic_id == APIC_ID_ALL) {
		for (i = 0; i < cpus_available; ++i)
			__lapic_set_lvt_mode(&lapic_list[i], pin, mode);
	} else {
		lapic = lapic_from_id(apic_id);
		__lapic_set_lvt_mode(lapic, pin, mode);
	}

	return 0;
}

/*
 * find_cpu_lapic:
 * Read the local APIC ID of the executing processor,
 * find the corresponding struct lapic, and save it.
 */
static void find_cpu_lapic(void)
{
	struct lapic *lapic;
	uint32_t eax, edx;
	int lapic_id;

	if (cpu_supports(CPUID_X2APIC)) {
		/* check if operating in X2APIC mode */
		rdmsr(IA32_APIC_BASE, &eax, &edx);
		if (eax & IA32_APIC_BASE_EXTD) {
			rdmsr(IA32_X2APIC_APICID, &eax, &edx);
			lapic_id = eax;
			goto find_lapic;
		}
	}

	lapic_id = lapic_reg_read(0x20) >> 24;

find_lapic:
	lapic = lapic_from_id(lapic_id);
	this_cpu_write(local_apic, lapic);
}


/*
 * apic_init:
 * Configure the LAPIC to send interrupts and enable it.
 */
void lapic_init(void)
{
	find_cpu_lapic();
	lapic_enable(lapic_phys_base);
	lapic_reg_write(0xF0, 0x100 | APIC_IRQ_SPURIOUS);
}

int bsp_apic_init(void)
{
	if (!cpu_supports(CPUID_APIC | CPUID_MSR) ||
	    (acpi_parse_madt() != 0 && parse_mp_tables() != 0)) {
		this_cpu_write(local_apic, NULL);
		cpus_available = 1;
		return 1;
	}

	lapic_virt_base = (addr_t)vmalloc(PAGE_SIZE);
	map_page_kernel(lapic_virt_base, lapic_phys_base,
	                PROT_WRITE, PAGE_CP_UNCACHEABLE);
	lapic_init();

	return 0;
}
