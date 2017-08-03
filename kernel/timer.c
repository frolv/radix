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

#include <radix/klog.h>
#include <radix/timer.h>

#define TIMER "TIMER: "

/* list of all timers in the system, sorted by decreasing rating */
static struct list system_timer_list = LIST_INIT(system_timer_list);

struct timer *system_timer = NULL;

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
	if (timer->rating < 1 || timer->rating > 100) {
		klog(KLOG_ERROR, TIMER "invalid rating provided for timer %s",
		     timer->name);
		return;
	}

	if (!system_timer) {
		list_add(&system_timer_list, &timer->timer_list);
		update_system_timer(timer);
	} else {
		timer_list_add(timer);
		if (timer->rating > system_timer->rating)
			update_system_timer(timer);
	}
}
