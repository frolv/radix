/*
 * include/radix/event.h
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

#ifndef RADIX_EVENT_H
#define RADIX_EVENT_H

#include <radix/task.h>
#include <radix/time.h>

#define MIN_EVENT_DELTA (50 * NSEC_PER_USEC)

void event_init(void);
void event_start(void);
void cpu_event_init(void);
void event_handler(void);

void timekeeping_event_set_period(uint64_t period);

int sched_event_add(uint64_t timestamp);
void sched_event_del(void);

int sleep_event_add(struct task *task, uint64_t timestamp);

#endif /* RADIX_EVENT_H */
