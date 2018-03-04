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
#include <radix/sched.h>
#include <radix/slab.h>
#include <radix/smp.h>
#include <radix/task.h>
#include <radix/timer.h>
#include <radix/time.h>

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
static DEFINE_PER_CPU(struct event *, dummy_event) = NULL;
static DEFINE_PER_CPU(struct event *, sched_event) = NULL;

static void __event_insert(struct event *evt);
static void __event_schedule(struct event *evt);

static void event_process(struct event *evt)
{
	switch (evt->type) {
	case EVENT_SCHED:
		this_cpu_write(sched_event, NULL);
		schedule(0);
		break;
	case EVENT_SLEEP:
		break;
	case EVENT_TIME:
		timer_accumulate();
		evt->time += evt->tk_period;
		__event_insert(evt);
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

	if (list_empty(eventq))
		return;

	__event_schedule(evt);
}

static void timekeeping_event_init(uint64_t period, uint64_t initial);

static void struct_event_init(void *p)
{
	struct event *evt = p;

	list_init(&evt->list);
}

void event_init(void)
{
	event_cache = create_cache("event", sizeof (struct event),
	                           SLAB_MIN_ALIGN, SLAB_PANIC,
	                           struct_event_init);
}

void event_start(void)
{
	timekeeping_event_init(system_timer->max_ns / 2,
	                       system_timer->max_ns / 4);
}

/*
 * __event_insert:
 * Insert the specified event into the event queue. If it gets inserted
 * at the front of the queue and a dummy event exists, remove the dummy
 * event.
 */
static void __event_insert(struct event *evt)
{
	struct list *eventq;
	struct event *curr, *prev;

	eventq = this_cpu_ptr(&event_queue);
	if (list_empty(eventq)) {
		list_add(eventq, &evt->list);
		return;
	}

	list_for_each_entry(curr, eventq, list) {
		if (curr->time > evt->time) {
			list_ins(&curr->list, &evt->list);
			if (curr->type == EVENT_DUMMY)
				list_del(&curr->list);
			goto check_prev;
		}
	}
	list_ins(eventq, &evt->list);

check_prev:
	if (evt->list.prev != eventq) {
		prev = list_prev_entry(evt, list);
		if (prev->type == EVENT_DUMMY)
			list_del(&prev->list);
	}
}

/*
 * __dummy_event:
 * Schedule a dummy event to occur after the specified period.
 * Dummy events don't do anything; they are used when the next real
 * event occurs after a period longer than the IRQ timer's max_ns.
 */
static void __dummy_event(uint64_t delta)
{
	struct event *dummy;
	struct list *eventq;

	dummy = this_cpu_read(dummy_event);
	eventq = this_cpu_ptr(&event_queue);

	list_add(eventq, &dummy->list);
	schedule_timer_irq(delta);
}

static void __event_schedule(struct event *evt)
{
	uint64_t now, delta, max_ns;

	now = time_ns();
	delta = max(evt->time - now, MIN_EVENT_DELTA);
	max_ns = irq_timer_max_ns();

	if (delta > max_ns)
		__dummy_event(max_ns - NSEC_PER_USEC);
	else
		schedule_timer_irq(delta);
}

/*
 * __event_add:
 * Insert an event into the event queue and schedule it if it is first.
 */
static void __event_add(struct event *evt)
{
	__event_insert(evt);
	if (evt->list.prev == this_cpu_ptr(&event_queue))
		__event_schedule(evt);
}

/*
 * __event_remove:
 * Remove the specified event from the event queue,
 * rescheduling the timer IRQ if necessary.
 */
static void __event_remove(struct event *evt)
{
	struct event *prev;
	struct list *eventq;
	int sched;

	eventq = this_cpu_ptr(&event_queue);
	sched = 0;

	if (evt->list.prev == eventq) {
		sched = 1;
	} else {
		prev = list_prev_entry(evt, list);
		if (prev->type == EVENT_DUMMY) {
			list_del(&prev->list);
			sched = 1;
		}
	}

	list_del(&evt->list);
	if (sched)
		__event_schedule(list_first_entry(eventq, struct event, list));
}

static struct event *tk_event = NULL;

/*
 * timekeeping_event_init:
 * Launch the timekeeping event, running with the specified period,
 * with the given initial delta until the first event occurs.
 */
static void timekeeping_event_init(uint64_t period, uint64_t initial)
{
	tk_event = event_alloc();
	if (IS_ERR(tk_event))
		panic("could not initialize kernel timekeeping event\n");

	tk_event->time = time_ns() + initial;
	tk_event->type = EVENT_TIME;
	tk_event->flags = EVENT_STATIC;
	tk_event->tk_period = period;

	__event_add(tk_event);
}

/*
 * timekeeping_event_update:
 * Change the period of the timekeeping event to the specified value.
 */
void timekeeping_event_update(uint64_t period)
{
	if (!tk_event)
		return;

	__event_remove(tk_event);
	tk_event->time = time_ns() + period;
	tk_event->tk_period = period;
	__event_add(tk_event);
}

/*
 * sched_event_add:
 * Insert a scheduler event to occur at the specified timestamp.
 */
int sched_event_add(uint64_t timestamp)
{
	struct event *evt;

	evt = event_alloc();
	if (IS_ERR(evt))
		return ERR_VAL(evt);

	evt->time = timestamp;
	evt->type = EVENT_SCHED;
	evt->flags = 0;

	__event_add(evt);
	this_cpu_write(sched_event, evt);

	return 0;
}

/*
 * sched_event_del:
 * Delete the active scheduler event.
 */
void sched_event_del(void)
{
	struct list *eventq;
	struct event *evt;

	evt = this_cpu_read(sched_event);
	if (!evt)
		return;

	eventq = this_cpu_ptr(&event_queue);

	/* sched event was first in queue; must schedule next event */
	if (evt->list.prev == eventq) {
		if (list_empty(eventq))
			schedule_timer_irq(0);
		else
			__event_schedule(list_first_entry(eventq, struct event, list));
	}

	list_del(&evt->list);
	this_cpu_write(sched_event, NULL);
	event_free(evt);
}

/*
 * cpu_event_init:
 * Initialize the per-CPU structures required for each CPU.
 */
void cpu_event_init(void)
{
	struct event *dummy;

	list_init(this_cpu_ptr(&event_queue));

	dummy = event_alloc();
	if (IS_ERR(dummy))
		panic("could not allocate dummy event for CPU %d\n",
		      processor_id());

	dummy->time = 0;
	dummy->type = EVENT_DUMMY;
	dummy->flags = EVENT_STATIC;
	this_cpu_write(dummy_event, dummy);
}
