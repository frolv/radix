/*
 * include/radix/timer.h
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

#ifndef RADIX_TIMER_H
#define RADIX_TIMER_H

#include <radix/list.h>
#include <radix/types.h>

struct timer {
	uint64_t        (*read)(void);
	uint32_t        mult;
	uint32_t        shift;
	unsigned long   frequency;
	int             (*start)(void);
	int             (*stop)(void);
	int             (*enable)(void);
	int             (*disable)(void);
	unsigned long   flags;
	const char      *name;
	int             rating;
	struct list     timer_list;
};

#define TIMER_ENABLED   (1 << 0)
#define TIMER_RUNNING   (1 << 1)

void timer_register(struct timer *timer);

#endif /* RADIX_TIMER_H */
