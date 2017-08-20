/*
 * kernel/timer.c
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

#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/time.h>
#include <radix/timer.h>

#define TIMER "timer: "

/* list of all timers in the system, sorted by decreasing rating */
static struct list system_timer_list = LIST_INIT(system_timer_list);

struct timer *system_timer = NULL;

static uint64_t time_ns_timer(void)
{
	uint64_t ticks;

	ticks = system_timer->read();
	return (ticks * system_timer->mult) >> system_timer->shift;
}

/*
 * __calc_mult_shift:
 * Calculate values of mult and shift which can be used to convert
 * frequency `from` to frequency `to` via `to = (from * mult) >> shift`.
 *
 * `secs` is the minimum number of seconds frequency `from` should be
 * allowed to run such that `from * mult` does not overflow an unsigned
 * 64-bit integer.
 */
static void __calc_mult_shift(uint32_t *mult, uint32_t *shift,
                              uint64_t from, uint64_t to,
                              unsigned int secs)
{
	uint64_t mlt, tmp;
	uint32_t sft, shift_acc;

	tmp = ((uint64_t)secs * from) >> 32;
	shift_acc = 32;
	while (tmp) {
		tmp >>= 1;
		--shift_acc;
	}

	for (sft = 32; sft > 0; --sft) {
		mlt = to << sft;
		mlt += from / 2;
		mlt /= from;
		if (!(mlt >> shift_acc))
			break;
	}

	*mult = mlt;
	*shift = sft;
}

/*
 * timer_configure:
 * Calculate mult, shift, max_ticks and max_ns for the specified timer.
 * max_ticks is the number of ticks the timer can run for before
 * `ticks * mult` overflows; max_ns is the number of nanoseconds the
 * timer can run before this overflow occurs.
 */
static void timer_configure(struct timer *timer)
{
	uint64_t max_ticks;

	/* timer did not provide its own mult/shift; must calculate them */
	if (!timer->mult)
		__calc_mult_shift(&timer->mult, &timer->shift, timer->frequency,
		                  NSEC_PER_SEC, 600);

	max_ticks = ~0ULL / timer->mult;
	if (timer->max_ticks)
		timer->max_ticks = min(timer->max_ticks, max_ticks);
	else
		timer->max_ticks = max_ticks;

	timer->max_ns = (timer->max_ticks * timer->mult) >> timer->shift;
}

/*
 * timer_list_add:
 * Insert the specified timer into the list of timers in the system.
 */
static void timer_list_add(struct timer *timer)
{
	struct timer *tm;

	list_for_each_entry(tm, &system_timer_list, timer_list) {
		if (timer->rating > tm->rating) {
			list_ins(&tm->timer_list, &timer->timer_list);
			return;
		}
	}
	list_ins(&system_timer_list, &timer->timer_list);
}

static void update_system_timer(struct timer *timer)
{
	if (!(timer->flags & TIMER_ENABLED)) {
		if (timer->enable() != 0) {
			klog(KLOG_WARNING, TIMER "failed to enable timer %s",
			     timer->name);
			return;
		}
	}

	if (system_timer) {
		if (system_timer->flags & TIMER_RUNNING)
			system_timer->stop();
		if (system_timer->flags & TIMER_ENABLED)
			system_timer->disable();
	}

	if (!(timer->flags & TIMER_RUNNING))
		timer->start();

	system_timer = timer;
	klog(KLOG_INFO, TIMER "system timer switched to %s", timer->name);
}

/*
 * timer_register:
 * Register a new timer source for the system. If it has a higher rating than
 * the active system timer, switch the system timer to the new source.
 */
void timer_register(struct timer *timer)
{
	if (timer->rating < 0 || timer->rating > 100) {
		klog(KLOG_ERROR, TIMER "invalid rating provided for timer %s",
		     timer->name);
		return;
	}

	timer_configure(timer);
	klog(KLOG_INFO, TIMER "%s max_ticks 0x%llX max_ns %llu",
	     timer->name, timer->max_ticks, timer->max_ns);

	if (!system_timer) {
		list_add(&system_timer_list, &timer->timer_list);
		update_system_timer(timer);
		time_ns = time_ns_timer;
	} else {
		timer_list_add(timer);
		if (timer->rating > system_timer->rating)
			update_system_timer(timer);
	}
}
