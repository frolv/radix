/*
 * arch/i386/setup.c
 * Copyright (C) 2021 Alexei Frolov
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

#include <radix/asm/apic.h>
#include <radix/cpu.h>
#include <radix/timer.h>

#include <acpi/acpi.h>

// Initializes x86-specific hardware features and data structures.
void arch_main_setup(void)
{
    acpi_init();
    bsp_init();

    hpet_register();
    acpi_pm_register();
    rtc_register();

    if (cpu_supports(CPUID_APIC) && apic_enabled()) {
        lapic_timer_calibrate();
        lapic_timer_register();
        pit_register();
    } else {
        // If there is no APIC, the PIT must be used as a scheduling timer.
        pit_oneshot_register();
    }
}
