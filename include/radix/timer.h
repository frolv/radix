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

#include <radix/asm/timer.h>
#include <radix/list.h>
#include <radix/types.h>

struct timer {
	uint64_t        (*read)(void);
	uint32_t        mult;
	uint32_t        shift;
	unsigned long   frequency;
	uint64_t        max_ticks;
	uint64_t        max_ns;
	uint64_t        (*reset)(void);
	void            (*start)(void);
	void            (*stop)(void);
	int             (*enable)(void);
	int             (*disable)(void);
	unsigned long   flags;
	const char      *name;
	int             rating;
	struct list     timer_list;
};

#define TIMER_ENABLED   (1 << 0)
#define TIMER_RUNNING   (1 << 1)
#define TIMER_EMULATED  (1 << 2)
#define TIMER_PERCPU    (1 << 3)

extern struct timer *system_timer;

void timer_register(struct timer *timer);
void timer_accumulate(void);


struct irq_timer {
	void            (*schedule_irq)(uint64_t);
	uint32_t        mult;
	uint32_t        shift;
	unsigned long   frequency;
	uint64_t        max_ticks;
	uint64_t        max_ns;
	int             (*enable)(void);
	int             (*disable)(void);
	unsigned long   flags;
	const char      *name;
};

extern void (*schedule_timer_irq)(uint64_t ns);
extern uint64_t (*irq_timer_max_ns)(void);
int set_irq_timer(struct irq_timer *irqt);


struct percpu_timer_data {
	uint32_t        mult;
	uint32_t        shift;
	unsigned long   frequency;
	uint64_t        max_ticks;
	uint64_t        max_ns;
};

void set_percpu_timer_data(struct percpu_timer_data *pcpu_data);
void set_percpu_irq_timer_data(struct percpu_timer_data *pcpu_data);
int cpu_timer_init(void);

void handle_timer_action(void);

#endif /* RADIX_TIMER_H */
