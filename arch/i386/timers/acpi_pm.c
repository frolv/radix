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

#include <radix/io.h>
#include <radix/spinlock.h>
#include <radix/timer.h>

#include <acpi/acpi.h>
#include <acpi/tables/fadt.h>

#define ACPI_PM_FREQUENCY 3579545
#define ACPI_PM_MULT      2288559
#define ACPI_PM_SHIFT     13

/*
 * The ACPI power management timer is a counter provided by the ACPI BIOS which
 * increments at a fixed rate of 3.579545 MHz. This gives it a resolution of
 * approximately 279ns, making it a very reasonable choice for a system timer
 * source.
 *
 * The fact that ACPI PM counter reads require port access lower its rating.
 * Despite this, it is still a solid choice as a system timer source.
 */

static uint32_t acpi_pm_port;
static uint32_t acpi_pm_prev_ticks = 0;
static uint32_t acpi_pm_total_ticks = 0;

static struct timer acpi_pm;

/*
 * acpi_pm_read:
 * Read the curent value of the ACPI PM counter.
 */
static uint64_t acpi_pm_read(void)
{
    uint32_t ticks;
    int diff;

    ticks = inl(acpi_pm_port);
    diff = ticks - acpi_pm_prev_ticks;

    /* If diff < 0, i.e. prev_ticks > ticks, the counter has overflowed. */
    if (diff < 0)
        acpi_pm_total_ticks += (acpi_pm.max_ticks - acpi_pm_prev_ticks) + ticks;
    else
        acpi_pm_total_ticks += diff;

    acpi_pm_prev_ticks = ticks;
    return acpi_pm_total_ticks;
}

static uint64_t acpi_pm_reset(void)
{
    uint64_t ticks;

    ticks = acpi_pm_read();
    acpi_pm_total_ticks = 0;

    return ticks;
}

static int acpi_pm_enable(void)
{
    acpi_pm_prev_ticks = inl(acpi_pm_port);
    return 0;
}

static int acpi_pm_disable(void) { return 0; }

static void acpi_pm_dummy(void) {}

static struct timer acpi_pm = {.read = acpi_pm_read,
                               .reset = acpi_pm_reset,
                               .mult = ACPI_PM_MULT,
                               .shift = ACPI_PM_SHIFT,
                               .frequency = ACPI_PM_FREQUENCY,
                               .start = acpi_pm_dummy,
                               .stop = acpi_pm_dummy,
                               .enable = acpi_pm_enable,
                               .disable = acpi_pm_disable,
                               .flags = 0,
                               .name = "acpi_pm",
                               .rating = 30,
                               .timer_list = LIST_INIT(acpi_pm.timer_list)};

void acpi_pm_register(void)
{
    struct acpi_fadt *fadt;

    fadt = acpi_find_table(ACPI_FADT_SIGNATURE);
    if (!fadt)
        return;

    acpi_pm_port = fadt->pm_tmr_blk;
    if (!acpi_pm_port)
        return;

    acpi_pm.max_ticks =
        (fadt->flags & ACPI_FADT_TMR_VAL_EXT) ? 0xFFFFFFFF : 0x00FFFFFF;

    timer_register(&acpi_pm);
}
