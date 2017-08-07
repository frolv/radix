/*
 * arch/i386/include/radix/asm/timer.h
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

#ifndef ARCH_I386_RADIX_TIMER_H
#define ARCH_I386_RADIX_TIMER_H

void acpi_pm_register(void);
void pit_register(void);
void rtc_register(void);
void hpet_register(void);

/* TODO: remove these once we get a proper event framework */
int pit_setup_periodic_irq(int hz, void (*action)(void));
void pit_stop_periodic_irq(void);

#endif /* ARCH_I386_RADIX_TIMER_H */
