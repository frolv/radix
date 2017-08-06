/*
 * arch/i386/timers/pit.c
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
#include <radix/io.h>
#include <radix/irq.h>
#include <radix/types.h>
#include <radix/timer.h>

#define PIT_CHANNEL_0_PORT      0x40
#define PIT_CHANNEL_1_PORT      0x41
#define PIT_CHANNEL_2_PORT      0x42
#define PIT_COMMAND_PORT        0x43

#define PIT_BCD                 (1 << 0)
#define PIT_MODE_TERMINAL       (0 << 1)
#define PIT_MODE_ONESHOT        (1 << 1)
#define PIT_MODE_RATE           (2 << 1)
#define PIT_MODE_SQUARE         (3 << 1)
#define PIT_MODE_SWSTROBE       (4 << 1)
#define PIT_MODE_HWSTROBE       (5 << 1)
#define PIT_ACCESS_MODE_LATCH   (0 << 4)
#define PIT_ACCESS_MODE_LOBYTE  (1 << 4)
#define PIT_ACCESS_MODE_HIBYTE  (2 << 4)
#define PIT_ACCESS_MODE_LO_HI   (3 << 4)
#define PIT_CHANNEL_0           (0 << 6)
#define PIT_CHANNEL_1           (1 << 6)
#define PIT_CHANNEL_2           (2 << 6)
#define PIT_READBACK            (3 << 6)

#define PIT_IRQ                 0
#define PIT_OSC_FREQ            1193182
#define PIT_COUNTER_FREQ        2048

#define PIT_TICK_DELTA          1001
#define PIT_MULT                15625
#define PIT_SHIFT               5

/*
 * The programmable interrupt timer (PIT) is a universal chip on x86 systems
 * which can be used as a timer source. The timer is a software emulated
 * counter, incremented by PIT interrupts running at a frequency of rougly
 * 2048Hz. Unlike the RTC, the PIT does not require any port I/O within its
 * interrupt handler, making its IRQ handling much quicker.
 *
 * However, the PIT oscillates at an unusual frequency which cannot be divided
 * to produce an exact rate of 2048Hz. There is a difference of rougly 0.1%
 * between the desired frequency and the actual interrupt frequency of the PIT.
 * To counter this, PIT ticks are processed in thousands. Instead of running the
 * counter at a rate of 2048 PIT ticks per second, it is run at 2048000 ticks
 * per second. Each PIT interrupt increments the tick counter by 1001 to
 * account for the inaccuracy in its interrupt rate.
 *
 * Since the PIT counter is software emulated and has a low resolution,
 * its rating is the second lowest out of all x86 timers.
 *
 * The PIT is not available as a timer source on all systems; if the system
 * does not have an APIC (and thus no local APIC timer), the PIT must be run
 * in one-shot mode as a scheduling timer instead.
 */

static uint64_t pit_ticks = 0;
static struct timer pit;

static uint64_t pit_read(void)
{
	return pit_ticks;
}

static void pit_tick_handler(__unused void *device)
{
	pit_ticks += PIT_TICK_DELTA;
}

static int pit_enable(void)
{
	uint16_t divisor;

	divisor = PIT_OSC_FREQ / PIT_COUNTER_FREQ;
	outb(PIT_COMMAND_PORT, PIT_CHANNEL_0 |
	                       PIT_ACCESS_MODE_LO_HI |
	                       PIT_MODE_SQUARE);
	outb(PIT_CHANNEL_0_PORT, divisor & 0xFF);
	outb(PIT_CHANNEL_0_PORT, (divisor >> 8) & 0xFF);

	if (request_fixed_irq(PIT_IRQ, &pit, pit_tick_handler) != 0)
		return 1;

	pit.flags |= TIMER_ENABLED;
	return 0;
}

static int pit_disable(void)
{
	release_irq(PIT_IRQ, &pit);
	pit.flags &= ~TIMER_ENABLED;
	return 0;
}

static void pit_start(void)
{
	unmask_irq(PIT_IRQ);
	pit.flags |= TIMER_RUNNING;
}

static void pit_stop(void)
{
	mask_irq(PIT_IRQ);
	pit.flags &= ~TIMER_RUNNING;
}

static struct timer pit = {
	.read           = pit_read,
	.mult           = PIT_MULT,
	.shift          = PIT_SHIFT,
	.frequency      = PIT_COUNTER_FREQ,
	.start          = pit_start,
	.stop           = pit_stop,
	.enable         = pit_enable,
	.disable        = pit_disable,
	.flags          = 0,
	.name           = "pit",
	.rating         = 2,
	.timer_list     = LIST_INIT(pit.timer_list)
};

void pit_register(void)
{
	timer_register(&pit);
}

/* TODO: remove everything below */
static void (*periodic_action)(void) = NULL;

static void pit_irq_handler(__unused void *device)
{
	periodic_action();
}

int pit_setup_periodic_irq(int hz, void (*action)(void))
{
	uint16_t divisor;

	divisor = PIT_OSC_FREQ / hz;
	outb(PIT_COMMAND_PORT, PIT_CHANNEL_0 |
	                       PIT_ACCESS_MODE_LO_HI |
	                       PIT_MODE_SQUARE);
	outb(PIT_CHANNEL_0_PORT, divisor & 0xFF);
	outb(PIT_CHANNEL_0_PORT, (divisor >> 8) & 0xFF);

	if (request_fixed_irq(PIT_IRQ, &pit, pit_irq_handler) != 0)
		return 1;

	periodic_action = action;
	unmask_irq(PIT_IRQ);

	return 0;
}

void pit_stop_periodic_irq(void)
{
	periodic_action = NULL;
	release_irq(PIT_IRQ, &pit);
}
