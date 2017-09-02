/*
 * kernel/event.c
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

#include <radix/compiler.h>
#include <radix/error.h>
#include <radix/event.h>
#include <radix/kernel.h>
#include <radix/list.h>
#include <radix/percpu.h>
#include <radix/slab.h>
#include <radix/task.h>
#include <radix/timer.h>
#include <radix/time.h>

#define MIN_EVENT_DELTA (25 * NSEC_PER_USEC)

enum event_type {
	EVENT_SCHED,
	EVENT_SLEEP,
	EVENT_TIME,
	EVENT_DUMMY
};

struct event {
	uint64_t                time;
	int                     type;
	unsigned long           flags;
	union {
		uint64_t        tk_period;
		struct task     *sl_task;
	};
	struct list             list;
};

#define EVENT_STATIC (1 << 0)

static struct slab_cache *event_cache;

static DEFINE_PER_CPU(struct list, event_queue);

static void event_process(struct event *evt)
{
	switch (evt->type) {
	case EVENT_SCHED:
		break;
	case EVENT_SLEEP:
		break;
	case EVENT_TIME:
		break;
	case EVENT_DUMMY:
		break;
	}
}

#define event_alloc() alloc_cache(event_cache)

static __always_inline void event_free(struct event *evt)
{
	if (!(evt->flags & EVENT_STATIC))
		free_cache(event_cache, evt);
}

void event_handler(void)
{
	struct list *eventq;
	struct event *evt;
	uint64_t now;

	eventq = this_cpu_ptr(&event_queue);
	evt = list_first_entry(eventq, struct event, list);
	list_del(&evt->list);
	event_process(evt);
	event_free(evt);

	/*
	 * Keep processing events in the queue until the next occurs
	 * at least MIN_EVENT_DELTA after the end of the current.
	 */
	while (!list_empty(eventq)) {
		evt = list_first_entry(eventq, struct event, list);
		now = time_ns();
		if (now < evt->time && evt->time - now > MIN_EVENT_DELTA)
			break;

		list_del(&evt->list);
		event_process(evt);
		event_free(evt);
	}
}

static void struct_event_init(void *p)
{
	struct event *evt = p;

	list_init(&evt->list);
}

void event_init(void)
{
	event_cache = create_cache("event", sizeof (struct event),
	                           SLAB_MIN_ALIGN, SLAB_PANIC,
	                           struct_event_init, struct_event_init);
}

/*
 * cpu_event_init:
 * Initialize the per-CPU structures required for each CPU.
 */
void cpu_event_init(void)
{
	list_init(this_cpu_ptr(&event_queue));
}
