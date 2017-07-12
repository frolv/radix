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
#include <radix/klog.h>
#include <radix/mm.h>

#include <rlibc/string.h>

#include "apic.h"

/* mp table i/o apic entries don't store irq base, so we need to keep track */
static int curr_ioapic_irq_base = 0;

static void __mp_processor(struct mp_table_processor *s)
{
	klog(KLOG_INFO, "MPS: LAPIC id %d %sactive",
	     s->apic_id, s->cpu_flags & MP_PROCESSOR_ACTIVE ? "" : "in");
}

static void __mp_bus(struct mp_table_bus *s)
{
	klog(KLOG_INFO, "MPS: bus id %d signature %.6s",
	     s->bus_id, s->bus_type);
}

static void __mp_ioapic(struct mp_table_io_apic *s)
{
	klog(KLOG_INFO, "MPS: I/O APIC id %d base %p irq_base %d",
	     s->ioapic_id, s->ioapic_base, curr_ioapic_irq_base);
}

static void __mp_io_interrupt(struct mp_table_io_interrupt *s)
{
	static struct ioapic *ioapic;

	klog(KLOG_INFO, "MPS: I/O INT bus %d int %d ioapic %d pin %d",
	     s->source_bus, s->source_irq, s->dest_ioapic, s->dest_intin);
}

static void __mp_local_int(struct mp_table_local_interrupt *s)
{
	/* TODO */
	(void)s;
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

int parse_mp_tables(void)
{
	struct mp_config_table *mp;
	uint8_t *s;
	size_t i;

	mp = find_mp_config_table();
	if (!mp)
		return 1;

	lapic_phys_base = mp->lapic_base;
	klog(KLOG_INFO, "MPS: local APIC %p", lapic_phys_base);

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
