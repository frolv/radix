/*
 * include/radix/sleep.h
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

#ifndef RADIX_SLEEP_H
#define RADIX_SLEEP_H

#include <radix/event.h>
#include <radix/time.h>

static inline void __sleep_busy(uint64_t ns)
{
    uint64_t end = time_ns() + ns;
    while (time_ns() < end) {}
}

void __sleep_blocking(uint64_t ns);

static inline void sleep(uint64_t ns)
{
    if (ns < MIN_EVENT_DELTA) {
        __sleep_busy(ns);
    } else {
        __sleep_blocking(ns);
    }
}

static inline void sleep_ms(uint64_t ms) { sleep(ms * NSEC_PER_MSEC); }
static inline void sleep_us(uint64_t us) { sleep(us * NSEC_PER_USEC); }

#endif  // RADIX_SLEEP_H
