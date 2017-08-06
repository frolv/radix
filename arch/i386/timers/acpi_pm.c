/*
 * arch/i386/timers/acpi_pm.c
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

#include <acpi/acpi.h>
#include <acpi/tables/fadt.h>

#include <radix/io.h>
#include <radix/spinlock.h>
#include <radix/timer.h>
/* TODO: remove this include */
#include <radix/cpu.h>

#define ACPI_PM_FREQUENCY       3579545
#define ACPI_PM_MULT            2288559
#define ACPI_PM_SHIFT           13

/*
 * The ACPI power management timer is a counter provided by the ACPI BIOS which
 * increments at a fixed rate of 3.579545 MHz. This gives it a resolution of
 * approximately 279ns, making it a very reasonable choice for a system timer
 * source.
 *
 * However, there is one problem with the ACPI PM timer: its counter is either
 * 32 bits or 24 bits long, causing it to overflow frequently. A 24-bit counter
 * will overflow every ~4.6s, and the 32-bit counter will overflow every ~20m.
 * Due to this, it is possible for tick information to be lost if the counter
 * overflows twice without a read occurring in between. While it is unlikely
 * that the system runs for 4.6 seconds without reading the time, the possiblity
 * of tick loss must be prevented. This is done by setting up an event to
 * periodically read the counter.
 *
 * The counter limitation lowers the rating of the ACPI PM timer, as well as
 * the fact that counter reads require port access. Despite this, it is still
 * a solid choice as a system timer source.
 */

static uint32_t acpi_pm_port;
static uint32_t acpi_pm_max_ticks;
static uint32_t acpi_pm_prev_ticks = 0;
static uint64_t acpi_pm_total_ticks = 0;

static spinlock_t acpi_pm_lock = SPINLOCK_INIT;

/*
 * acpi_pm_read:
 * Read the curent value of the ACPI PM counter and update total ticks.
 */
static uint64_t acpi_pm_read(void)
{
	uint32_t ticks;
	int diff;

	spin_lock(&acpi_pm_lock);
	ticks = inl(acpi_pm_port);
	diff = ticks - acpi_pm_prev_ticks;

	/* If diff < 0, i.e. prev_ticks > ticks, the counter has overflowed. */
	if (diff < 0)
		acpi_pm_total_ticks += (acpi_pm_max_ticks - acpi_pm_prev_ticks)
		                       + ticks;
	else
		acpi_pm_total_ticks += diff;

	acpi_pm_prev_ticks = ticks;
	spin_unlock(&acpi_pm_lock);

	return acpi_pm_total_ticks;
}

static void acpi_pm_update(void)
{
	acpi_pm_read();
}

/*
 * acpi_pm_enable:
 * Nothing needs to be done to enable the ACPI PM timer counter as it is
 * always running. However, in order to ensure that a two counter overflows
 * do not occur without a read in between (causing ticks to be lost), an
 * event is set up to read the current counter value four times every second.
 */
static int acpi_pm_enable(void)
{
	acpi_pm_prev_ticks = inl(acpi_pm_port);

	/* TODO: replace this when we get a proper event framework */
	return pit_setup_periodic_irq(4, acpi_pm_update);
}

static int acpi_pm_disable(void)
{
	pit_stop_periodic_irq();
	return 0;
}

static void acpi_pm_dummy(void)
{
}

static struct timer acpi_pm = {
	.read           = acpi_pm_read,
	.mult           = ACPI_PM_MULT,
	.shift          = ACPI_PM_SHIFT,
	.frequency      = ACPI_PM_FREQUENCY,
	.start          = acpi_pm_dummy,
	.stop           = acpi_pm_dummy,
	.enable         = acpi_pm_enable,
	.disable        = acpi_pm_disable,
	.flags          = 0,
	.name           = "acpi_pm",
	.rating         = 30,
	.timer_list     = LIST_INIT(acpi_pm.timer_list)
};

void acpi_pm_register(void)
{
	struct acpi_fadt *fadt;

	fadt = acpi_find_table(ACPI_FADT_SIGNATURE);
	if (!fadt)
		return;

	acpi_pm_port = fadt->pm_tmr_blk;
	if (!acpi_pm_port)
		return;

	acpi_pm_max_ticks =
		(fadt->flags & ACPI_FADT_TMR_VAL_EXT) ? 0xFFFFFFFF : 0x00FFFFFF;

	timer_register(&acpi_pm);
}
