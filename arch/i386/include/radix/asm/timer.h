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

#include <radix/types.h>

void acpi_pm_register(void);
void pit_register(void);
void rtc_register(void);
void hpet_register(void);

void pit_oneshot_register(void);
int pit_wait_setup(void);
void pit_wait_finish(void);
void pit_wait(uint32_t us);

#endif /* ARCH_I386_RADIX_TIMER_H */
