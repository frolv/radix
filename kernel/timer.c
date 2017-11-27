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

#include <radix/compiler.h>
#include <radix/cpu.h>
#include <radix/event.h>
#include <radix/ipi.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/percpu.h>
#include <radix/smp.h>
#include <radix/spinlock.h>
#include <radix/time.h>
#include <radix/timer.h>

#define TIMER "timer: "

/* list of all timers in the system, sorted by decreasing rating */
static struct list system_timer_list = LIST_INIT(system_timer_list);

struct timer *system_timer = NULL;
static struct irq_timer *sys_irq_timer = NULL;

DEFINE_PER_CPU(struct percpu_timer_data *, pcpu_timer) = NULL;
DEFINE_PER_CPU(struct percpu_timer_data *, pcpu_irq_timer) = NULL;

static spinlock_t time_ns_lock = SPINLOCK_INIT;
static uint64_t ns_since_boot = 0;

enum {
	TIMER_ACTION_ENABLE,
	TIMER_ACTION_DISABLE,
	TIMER_ACTION_UPDATE
};

#define TIMER_ACTION_IRQ_TIMER (1 << 31)

#define TIMER_ACTION_COMPLETE  (1 << 30)
#define TIMER_ACTION_FAILED    (1 << 31)

static struct {
	int             action;
	int		state;
	void            *timer;
	void            *new_timer;
	cpumask_t	mask;
} timer_action;

/*
 * time_ns_static:
 * Temporary time_ns function which returns the last known system time.
 * Used if no timers in the system are active.
 */
static uint64_t time_ns_static(void)
{
	return ns_since_boot;
}

static uint64_t time_ns_timer(void)
{
	uint64_t ticks, initial_ns, current_ns;
	unsigned long irqstate;

	spin_lock_irq(&time_ns_lock, &irqstate);
	ticks = system_timer->read();
	initial_ns = ns_since_boot;
	current_ns = (ticks * system_timer->mult) >> system_timer->shift;
	spin_unlock_irq(&time_ns_lock, irqstate);

	return initial_ns + current_ns;
}

/*
 * timer_accumulate:
 * Update the `ns_since_boot` variable using the current timer count,
 * then reset the timer's ticks to zero.
 * This is done periodically to prevent `ticks * mult` from overflowing.
 */
void timer_accumulate(void)
{
	uint64_t ticks;
	unsigned long irqstate;

	spin_lock_irq(&time_ns_lock, &irqstate);
	ticks = system_timer->reset();
	ns_since_boot += (ticks * system_timer->mult) >> system_timer->shift;
	spin_unlock_irq(&time_ns_lock, irqstate);
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

static int timer_enable(struct timer *timer)
{
	int err;

	if ((err = timer->enable()) != 0)
		return err;

	if (timer->flags & TIMER_PERCPU) {
		if (!this_cpu_read(pcpu_timer))
			return 1;
	} else {
		this_cpu_write(pcpu_timer, NULL);
	}

	if (!(timer->flags & TIMER_RUNNING))
		timer->start();

	return 0;
}

static void timer_disable(struct timer *timer)
{
	if (timer->flags & TIMER_RUNNING)
		timer->stop();
	if (timer->flags & TIMER_ENABLED)
		timer->disable();
}

/*
 * __timer_action_wait:
 * Wait for all other CPUs to complete a timer action.
 */
static __always_inline void __timer_action_wait(void)
{
	cpumask_t online;

	while (1) {
		online = cpumask_online();
		if ((timer_action.mask & online) == online)
			break;
	}

	timer_action.state |= TIMER_ACTION_COMPLETE;
}

/*
 * enable_percpu_timer:
 * Enable the specified timer source across all CPUs in the system.
 * Returns 1 if any processors fail to enable the timer.
 */
static int enable_percpu_timer(struct timer *timer)
{
	timer_action.action = TIMER_ACTION_ENABLE;
	timer_action.state = 0;
	timer_action.timer = timer;
	timer_action.mask = CPUMASK_SELF;
	send_timer_ipi();

	if (timer_enable(timer) != 0)
		timer_action.state |= TIMER_ACTION_FAILED;

	__timer_action_wait();

	/*
	 * During normal system operation, we can assume that a timer change
	 * will only occur if the current system timer has failed or become
	 * unreliable. If enabling a new timer fails, other CPUs in the
	 * system could continue running with an invalid timer.
	 * Until a new timer source is found, the time_ns function is set to
	 * return the last known system time.
	 */
	if (timer_action.state & TIMER_ACTION_FAILED) {
		time_ns = time_ns_static;
		return 1;
	}

	return 0;
}

/*
 * disable_percpu_timer:
 * Disable a per-CPU timer source across all processors in the system.
 */
static void disable_percpu_timer(struct timer *timer)
{
	timer_action.action = TIMER_ACTION_DISABLE;
	timer_action.state = 0;
	timer_action.timer = timer;
	timer_action.mask = CPUMASK_SELF;
	send_timer_ipi();

	timer_disable(timer);
	__timer_action_wait();
}

/*
 * update_percpu_timer:
 * Switch from one per-CPU timer to another, disabling the old one
 * and enabling the new across all processors in the system.
 */
static int update_percpu_timer(struct timer *old, struct timer *new)
{
	timer_action.action = TIMER_ACTION_UPDATE;
	timer_action.timer = old;
	timer_action.new_timer = new;
	send_timer_ipi();

	timer_disable(old);
	if (timer_enable(new) != 0)
		timer_action.state |= TIMER_ACTION_FAILED;

	__timer_action_wait();

	if (timer_action.state & TIMER_ACTION_FAILED) {
		time_ns = time_ns_static;
		return 1;
	}

	return 0;
}

/*
 * update_system_timer:
 * Switch the system timer to the specified timer.
 */
static int update_system_timer(struct timer *timer)
{
	int (*enable_fn)(struct timer *);

	if ((timer->flags & TIMER_PERCPU) &&
	    (system_timer->flags & TIMER_PERCPU))
		return update_percpu_timer(system_timer, timer);

	if (!(timer->flags & TIMER_ENABLED)) {
		enable_fn = (timer->flags & TIMER_PERCPU)
		          ? enable_percpu_timer
		          : timer_enable;

		if (enable_fn(timer) != 0) {
			klog(KLOG_WARNING, TIMER "failed to enable timer %s",
			     timer->name);
			return 1;
		}
	}

	if (system_timer) {
		timer_accumulate();

		if (system_timer->flags & TIMER_PERCPU)
			disable_percpu_timer(system_timer);
		else
			timer_disable(system_timer);

		timekeeping_event_update(timer->max_ns / 2);
	}

	system_timer = timer;
	klog(KLOG_INFO, TIMER "system timer switched to %s", timer->name);

	return 0;
}

/*
 * timer_register:
 * Register a new timer source for the system. If it has a higher rating than
 * the active system timer, switch the system timer to the new source.
 */
void timer_register(struct timer *timer)
{
	if (!timer)
		return;

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

struct irq_timer *system_irq_timer(void)
{
	return sys_irq_timer;
}

/*
 * set_irq_timer:
 * Set the specified IRQ timer as the active system IRQ timer.
 */
int set_irq_timer(struct irq_timer *irqt)
{
	uint64_t max_ticks;

	if (!irqt)
		return 1;

	if (sys_irq_timer == irqt)
		return 0;

	if (!irqt->mult)
		__calc_mult_shift(&irqt->mult, &irqt->shift, NSEC_PER_SEC,
				  irqt->frequency, 60);

	max_ticks = ~0ULL / irqt->mult;
	if (irqt->max_ticks)
		irqt->max_ticks = min(irqt->max_ticks, max_ticks);
	else
		irqt->max_ticks = max_ticks;

	irqt->max_ns = (irqt->max_ticks << irqt->shift) / irqt->mult;

	if (irqt->enable() != 0)
		return 1;

	if (sys_irq_timer) {
		/* TODO: make next event disable current irq_timer */
	}

	sys_irq_timer = irqt;
	return 0;
}

/*
 * __calc_pcpu_data:
 * Calculate timer values in the specified percpu_timer_data
 * struct based on the timer's frequency.
 */
static void __calc_pcpu_data(struct percpu_timer_data *pd)
{
	uint64_t max_ticks;

	if (!pd)
		return;

	if (!pd->mult)
		__calc_mult_shift(&pd->mult, &pd->shift, pd->frequency,
		                  NSEC_PER_SEC, 600);

	max_ticks = ~0ULL / pd->mult;
	if (pd->max_ticks)
		pd->max_ticks = min(pd->max_ticks, max_ticks);
	else
		pd->max_ticks = max_ticks;

	pd->max_ns = (pd->max_ticks << pd->shift) / pd->mult;
}

void set_percpu_timer_data(struct percpu_timer_data *pcpu_data)
{
	__calc_pcpu_data(pcpu_data);
	this_cpu_write(pcpu_timer, pcpu_data);
}

void set_percpu_irq_timer_data(struct percpu_timer_data *pcpu_data)
{
	__calc_pcpu_data(pcpu_data);
	this_cpu_write(pcpu_irq_timer, pcpu_data);
}

/*
 * handle_timer_action:
 * Handler for the timer action IPI. Called by all CPUs in
 * the system except for the one issuing the action.
 * There are three types of timer actions, all of which can
 * apply to either a system timer or an IRQ timer.
 *
 * UPDATE       Replace a running per-CPU timer with a new per-CPU timer
 * ENABLE       Enable a per-CPU timer
 * DISABLE      Disable a per-CPU timer
 */
void handle_timer_action(void)
{
	if (timer_action.action & TIMER_ACTION_IRQ_TIMER) {
		/* TODO: add timer actions for IRQ timers */
		switch (timer_action.action & 0xF) {
		case TIMER_ACTION_UPDATE:
			/* fallthrough */
		case TIMER_ACTION_ENABLE:
			break;
		case TIMER_ACTION_DISABLE:
			break;
		}
	} else {
		switch (timer_action.action & 0xF) {
		case TIMER_ACTION_UPDATE:
			timer_disable(timer_action.timer);
			/* fallthrough */
		case TIMER_ACTION_ENABLE:
			if (timer_enable(timer_action.timer) != 0)
				timer_action.state |= TIMER_ACTION_FAILED;
			break;
		case TIMER_ACTION_DISABLE:
			timer_disable(timer_action.timer);
			break;
		}
	}

	timer_action.mask |= CPUMASK_SELF;

	while (!(timer_action.state & TIMER_ACTION_COMPLETE))
		cpu_pause();
}
