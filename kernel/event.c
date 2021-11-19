/*
 * kernel/event.c
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

#include <radix/assert.h>
#include <radix/compiler.h>
#include <radix/error.h>
#include <radix/event.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/list.h>
#include <radix/percpu.h>
#include <radix/sched.h>
#include <radix/slab.h>
#include <radix/smp.h>
#include <radix/task.h>
#include <radix/time.h>
#include <radix/timer.h>

#include <stdbool.h>

#define EVENT "event: "

enum event_type {
    EVENT_SCHED,
    EVENT_SLEEP,
    EVENT_TIME,
    EVENT_DUMMY,
};

struct event {
    uint64_t timestamp;
    unsigned long flags;

    union {
        // Period for a timekeeping event.
        uint64_t tk_period;

        // Task to wake for a sleep event.
        struct task *sl_task;
    };

    struct list list;
};

#define EVENT_STATIC    (1 << 2)
#define EVENT_TYPE(evt) ((evt)->flags & 0x3)

static struct slab_cache *event_cache;

// A linked list of time-based events to run, ordered by increasing timestamp.
// TODO(frolv): This is not scalable.
static DEFINE_PER_CPU(struct list, event_queue);
static DEFINE_PER_CPU(spinlock_t, event_lock);

static DEFINE_PER_CPU(struct event *, dummy_event) = NULL;
static DEFINE_PER_CPU(struct event *, sched_event) = NULL;

static void __event_insert(struct event *evt);
static void __event_schedule(struct event *evt);

// Processes a single event.
static void event_process(struct event *evt)
{
    switch (EVENT_TYPE(evt)) {
    case EVENT_SCHED:
        this_cpu_write(sched_event, NULL);
        schedule(SCHED_SELECT);
        break;

    case EVENT_SLEEP:
        // TODO(frolv): Implement this.
        break;

    case EVENT_TIME:
        timer_accumulate();
        evt->timestamp += evt->tk_period;
        __event_insert(evt);
        break;

    case EVENT_DUMMY:
        // Nothing to do here.
        break;
    }
}

static __always_inline struct event *event_alloc()
{
    return alloc_cache(event_cache);
}

static __always_inline void event_free(struct event *evt)
{
    if (!(evt->flags & EVENT_STATIC)) {
        free_cache(event_cache, evt);
    }
}

// Main handler function called when an event interrupt occurs.
void event_handler(void)
{
    unsigned long irqstate;
    irq_save(irqstate);

    struct list *eventq = raw_cpu_ptr(&event_queue);

    // Process events in the queue until the next occurs at least
    // MIN_EVENT_DELTA after the end of the current.
    struct event *evt;
    while (!list_empty(eventq)) {
        evt = list_first_entry(eventq, struct event, list);
        uint64_t now = time_ns();
        if (now < evt->timestamp && evt->timestamp - now > MIN_EVENT_DELTA) {
            break;
        }

        list_del(&evt->list);
        event_process(evt);
        event_free(evt);
    }

    // If there are more events to run, schedule the first.
    if (!list_empty(eventq)) {
        __event_schedule(evt);
    }

    irq_restore(irqstate);
}

static void timekeeping_event_init(uint64_t period, uint64_t initial);

static void struct_event_init(void *p)
{
    struct event *evt = p;

    list_init(&evt->list);
}

void event_init(void)
{
    event_cache = create_cache("event",
                               sizeof(struct event),
                               SLAB_MIN_ALIGN,
                               SLAB_PANIC,
                               struct_event_init);
}

void event_start(void)
{
    timekeeping_event_init(system_timer->max_ns / 2, system_timer->max_ns / 4);
}

// Inserts an event into the event queue. If it is inserted at the front of the
// and a dummy event exists, replace the dummy event.
//
// Precondition: This function must be called with the event lock held.
static void __event_insert(struct event *evt)
{
    assert(evt);

    struct list *eventq = raw_cpu_ptr(&event_queue);
    if (list_empty(eventq)) {
        list_add(eventq, &evt->list);
        return;
    }

    struct event *curr;
    list_for_each_entry (curr, eventq, list) {
        if (curr->timestamp > evt->timestamp) {
            list_ins(&curr->list, &evt->list);
            if (EVENT_TYPE(curr) == EVENT_DUMMY) {
                list_del(&curr->list);
            }
            goto check_prev;
        }
    }
    list_ins(eventq, &evt->list);

check_prev:
    if (evt->list.prev != eventq) {
        struct event *prev = list_prev_entry(evt, list);
        if (EVENT_TYPE(prev) == EVENT_DUMMY) {
            list_del(&prev->list);
        }
    }
}

// Schedules a dummy event to occur after the specified period. Dummy events
// don't do anything; they are used as placeholderswhen the next real event
// occurs after a period longer than the IRQ timer's max_ns.
//
// Precondition: This is called with event_lock held.
static void __schdule_dummy_event(uint64_t delta)
{
    struct event *dummy = raw_cpu_read(dummy_event);
    struct list *eventq = raw_cpu_ptr(&event_queue);

    list_add(eventq, &dummy->list);
    schedule_timer_irq(delta);
}

static void __event_schedule(struct event *evt)
{
    assert(evt);

    uint64_t now = time_ns();
    uint64_t delta = max(evt->timestamp - now, MIN_EVENT_DELTA);
    uint64_t max_ns = irq_timer_max_ns();

    if (delta > max_ns) {
        __schdule_dummy_event(max_ns - NSEC_PER_USEC);
    } else {
        schedule_timer_irq(delta);
    }
}

// Inserts an event into the event queue, scheduling it if it is first.
static void __event_add(struct event *evt)
{
    unsigned long irqstate;
    spin_lock_irq(&event_lock, &irqstate);

    __event_insert(evt);
    if (evt->list.prev == this_cpu_ptr(&event_queue)) {
        __event_schedule(evt);
    }

    spin_unlock_irq(&event_lock, irqstate);
}

// Removes the specified event from the event queue. If the event was first in
// line, reschedules the timer IRQ for the next event.
static void __event_remove(struct event *evt)
{
    assert(evt);

    bool reschedule = false;
    unsigned long irqstate;

    spin_lock_irq(this_cpu_ptr(&event_lock), &irqstate);
    struct list *eventq = raw_cpu_ptr(&event_queue);

    if (evt->list.prev == eventq) {
        // This is the first event in the queue. Must reschedule timer.
        reschedule = true;
    } else {
        // Check if the previous event was a dummy for this event. If it
        // was, remove the dummy and reschedule for the next event.
        struct event *prev = list_prev_entry(evt, list);
        if (EVENT_TYPE(prev) == EVENT_DUMMY) {
            list_del(&prev->list);
            reschedule = true;
        }
    }

    list_del(&evt->list);

    if (reschedule) {
        if (list_empty(eventq)) {
            schedule_timer_irq(0);
        } else {
            evt = list_first_entry(eventq, struct event, list);
            __event_schedule(evt);
        }
    }

    spin_unlock_irq(this_cpu_ptr(&event_lock), irqstate);
}

static struct event *tk_event = NULL;

// Launch the timekeeping event, running with the specified period and a given
// initial delta until the first event occurs. This should only be called once.
static void timekeeping_event_init(uint64_t period, uint64_t initial)
{
    assert(!tk_event);

    tk_event = event_alloc();
    if (IS_ERR(tk_event)) {
        panic("Failed to allocate kernel timekeeping event");
    }

    tk_event->timestamp = time_ns() + initial;
    tk_event->flags = EVENT_STATIC | EVENT_TIME;
    tk_event->tk_period = period;

    klog(KLOG_INFO, EVENT "initializing kernel timekeeping event");
    __event_add(tk_event);
}

// Changes the period of the timekeeping event to the specified value.
void timekeeping_event_set_period(uint64_t period)
{
    if (!tk_event) {
        return;
    }

    __event_remove(tk_event);
    tk_event->timestamp = time_ns() + period;
    tk_event->tk_period = period;
    __event_add(tk_event);
}

// Inserts a scheduler event at the specified timestamp.
int sched_event_add(uint64_t timestamp)
{
    // Remove an existing scheduler event, as there can only be one.
    if (this_cpu_read(sched_event) != NULL) {
        sched_event_del();
    }

    struct event *evt = event_alloc();
    if (IS_ERR(evt)) {
        return ERR_VAL(evt);
    }

    evt->timestamp = timestamp;
    evt->flags = EVENT_SCHED;

    __event_add(evt);
    this_cpu_write(sched_event, evt);

    return 0;
}

// Deletes an active scheduler event, if any.
void sched_event_del(void)
{
    struct event *evt = this_cpu_read(sched_event);
    if (!evt) {
        return;
    }

    __event_remove(evt);

    this_cpu_write(sched_event, NULL);
    event_free(evt);
}

// Initialize per-CPU event structures and data. Must be run by each CPU in the
// system separately.
void cpu_event_init(void)
{
    struct event *dummy;

    list_init(this_cpu_ptr(&event_queue));

    dummy = event_alloc();
    if (IS_ERR(dummy)) {
        panic("Failed to allocate dummy event for CPU %d", processor_id());
    }

    dummy->timestamp = 0;
    dummy->flags = EVENT_STATIC | EVENT_DUMMY;
    this_cpu_write(dummy_event, dummy);
}
