/*
 * arch/i386/cpu/mps.c
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

#include <radix/asm/mps.h>
#include <radix/asm/bios.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/mm.h>
#include <radix/slab.h>

#include <rlibc/string.h>

#include "apic.h"

#define MPS "MPS: "

/* array of all buses in the system */
static uint8_t *mp_buses;
static int mp_max_bus_id = 0;

static void __mp_processor(struct mp_table_processor *s)
{
	klog(KLOG_INFO, MPS "LAPIC id %d %sactive",
	     s->apic_id, s->cpu_flags & MP_PROCESSOR_ACTIVE ? "" : "in");
}

static void __mp_bus(struct mp_table_bus *s)
{
	if (memcmp(s->bus_type, MP_BUS_SIGNATURE_ISA, 6) == 0)
		mp_buses[s->bus_id] = BUS_TYPE_ISA;
	else if (memcmp(s->bus_type, MP_BUS_SIGNATURE_EISA, 6) == 0)
		mp_buses[s->bus_id] = BUS_TYPE_EISA;
	else if (memcmp(s->bus_type, MP_BUS_SIGNATURE_PCI, 6) == 0)
		mp_buses[s->bus_id] = BUS_TYPE_PCI;
	else
		mp_buses[s->bus_id] = BUS_TYPE_UNKNOWN;

	klog(KLOG_INFO, MPS "bus id %d signature %.6s",
	     s->bus_id, s->bus_type);
}

/* mp table i/o apic entries don't store irq base, so we need to keep track */
static int curr_ioapic_irq_base = 0;

static void __mp_ioapic(struct mp_table_io_apic *s)
{
	struct ioapic *ioapic;

	klog(KLOG_INFO, MPS "I/O APIC id %d base %p irq_base %d",
	     s->ioapic_id, s->ioapic_base, curr_ioapic_irq_base);

	ioapic = ioapic_add(s->ioapic_id, s->ioapic_base, curr_ioapic_irq_base);
	if (!ioapic) {
		klog(KLOG_WARNING, MPS
		     "maximum supported number of I/O APICs reached, ignoring");
	} else {
		curr_ioapic_irq_base += ioapic->irq_count;
	}
}

static void __mp_io_interrupt(struct mp_table_io_interrupt *s)
{
	struct ioapic *ioapic;
	const char *type;

	if (s->dest_ioapic == 0xFF) {
		/*
		 * The interrupt is connected to the specified pin
		 * on all I/O APICs in the system.
		 * If only one I/O APIC exists, use it. Otherwise, ignore.
		 */
		if (ioapics_available != 1) {
			klog(KLOG_ERROR, MPS "ignoring I/O INT for pin %d",
			     s->dest_intin);
			return;
		}
		ioapic = ioapic_from_vector(0);
	} else {
		ioapic = ioapic_from_id(s->dest_ioapic);
	}

	if (!ioapic) {
		klog(KLOG_ERROR,
		     MPS "ignoring I/O INT for non-existent I/O APIC %d",
		     s->dest_ioapic);
		return;
	}

	switch (s->interrupt_type) {
	case MP_INTERRUPT_TYPE_INT:
		switch (mp_buses[s->source_bus]) {
		case BUS_TYPE_ISA:
		case BUS_TYPE_EISA:
			ioapic_set_bus(ioapic, s->dest_intin,
			               mp_buses[s->source_bus]);
			ioapic_set_vector(ioapic, s->dest_intin,
			                  s->source_irq);
			break;
		case BUS_TYPE_PCI:
		case BUS_TYPE_UNKNOWN:
			ioapic_set_bus(ioapic, s->dest_intin,
			               mp_buses[s->source_bus]);
			break;
		default:
			klog(KLOG_ERROR,
			     MPS "ignoring I/O INT from missing bus %d",
			     s->source_bus);
			return;
		}
		type = "INT";
		break;
	case MP_INTERRUPT_TYPE_NMI:
		ioapic_set_nmi(ioapic, s->dest_intin);
		type = "NMI";
		break;
	case MP_INTERRUPT_TYPE_SMI:
		ioapic_set_smi(ioapic, s->dest_intin);
		type = "SMI";
		break;
	case MP_INTERRUPT_TYPE_EXTINT:
		type = "EXTINT";
		ioapic_set_extint(ioapic, s->dest_intin);
		break;
	default:
		klog(KLOG_ERROR, MPS "ignoring unknown I/O INT type %d",
		     s->interrupt_type);
		return;
	}

	klog(KLOG_INFO, MPS "I/O INT bus %d int %d ioapic %d pin %d type %s",
	     s->source_bus, s->source_irq, s->dest_ioapic, s->dest_intin, type);
}

static void __mp_local_interrupt(struct mp_table_local_interrupt *s)
{
	/* TODO */
	klog(KLOG_INFO, MPS "Local INT bus %d int %d lapic %d pin %d",
	     s->source_bus, s->source_irq, s->dest_lapic, s->dest_lintin);
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
static struct mp_config_table *find_mp_config_table(void)
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

/*
 * mp_walk:
 * Iterate over all entries in the MP config table pointed to by `mp`,
 * calling `entry_handler` on each.
 */
static void mp_walk(struct mp_config_table *mp, void (*entry_handler)(void *))
{
	uint8_t *s;
	size_t i;

	s = (uint8_t *)(mp + 1);
	for (i = 0; i < mp->entry_count; ++i) {
		entry_handler(s);

		switch (*s) {
		case MP_TABLE_PROCESSOR:
			s += 20;
			break;
		case MP_TABLE_BUS:
		case MP_TABLE_IO_APIC:
		case MP_TABLE_IO_INTERRUPT:
		case MP_TABLE_LOCAL_INTERRUPT:
			s += 8;
			break;
		}
	}
}

static void mp_count_handler(void *entry)
{
	struct mp_table_bus *bus;

	switch (*(uint8_t *)entry) {
	case MP_TABLE_BUS:
		bus = entry;
		mp_max_bus_id = max(mp_max_bus_id, bus->bus_id);
		break;
	}
}

static void mp_parse_handler(void *entry)
{
	switch (*(uint8_t *)entry) {
	case MP_TABLE_PROCESSOR:
		__mp_processor((struct mp_table_processor *)entry);
		break;
	case MP_TABLE_BUS:
		__mp_bus((struct mp_table_bus *)entry);
		break;
	case MP_TABLE_IO_APIC:
		__mp_ioapic((struct mp_table_io_apic *)entry);
		break;
	case MP_TABLE_IO_INTERRUPT:
		__mp_io_interrupt((struct mp_table_io_interrupt *)entry);
		break;
	case MP_TABLE_LOCAL_INTERRUPT:
		__mp_local_interrupt((struct mp_table_local_interrupt *)entry);
		break;
	}
}

int parse_mp_tables(void)
{
	struct mp_config_table *mp;
	int i;

	mp = find_mp_config_table();
	if (!mp)
		return 1;

	lapic_phys_base = mp->lapic_base;
	klog(KLOG_INFO, MPS "local APIC %p", lapic_phys_base);

	/* count buses in the system */
	mp_walk(mp, mp_count_handler);
	mp_buses = kmalloc((mp_max_bus_id + 1) * sizeof *mp_buses);
	for (i = 0; i <= mp_max_bus_id; ++i)
		mp_buses[i] = BUS_TYPE_NONE;

	mp_walk(mp, mp_parse_handler);

	return 0;
}
