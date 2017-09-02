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

#include <radix/asm/event.h>
#include <radix/asm/idt.h>

#include <radix/compiler.h>
#include <radix/io.h>
#include <radix/irq.h>
#include <radix/types.h>
#include <radix/time.h>
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
#define PIT_COUNTER_FREQ        2048000
#define PIT_IRQ_FREQ            2048

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

static __always_inline void pit_data_port_write(int port, uint16_t val)
{
	outb(port, val & 0xFF);
	outb(port, (val >> 8) & 0xFF);
}

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

	divisor = PIT_OSC_FREQ / PIT_IRQ_FREQ;
	outb(PIT_COMMAND_PORT, PIT_CHANNEL_0 |
	                       PIT_ACCESS_MODE_LO_HI |
	                       PIT_MODE_SQUARE);
	pit_data_port_write(PIT_CHANNEL_0_PORT, divisor);

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
	.flags          = TIMER_EMULATED,
	.name           = "pit",
	.rating         = 2,
	.timer_list     = LIST_INIT(pit.timer_list)
};

/* pit_register: register the PIT timer source */
void pit_register(void)
{
	timer_register(&pit);
}


/*
 * The PIT can also be run in oneshot mode as an IRQ timer for the kernel.
 * This is only done on very old systems which do not have an APIC, and is
 * quite inefficient, requiring two legacy IO port writes per IRQ.
 */

static struct irq_timer pit_oneshot;

static void pit_schedule_irq(uint64_t ticks)
{
	pit_data_port_write(PIT_CHANNEL_0_PORT, ticks);
}

/* pit_oneshot_enable: set the PIT to run in one-shot mode */
static int pit_oneshot_enable(void)
{
	outb(PIT_COMMAND_PORT, PIT_CHANNEL_0 |
	                       PIT_ACCESS_MODE_LO_HI |
	                       PIT_MODE_TERMINAL);
	pit_data_port_write(PIT_CHANNEL_0_PORT, 0);

	idt_set(IRQ_TO_VECTOR(PIT_IRQ), event_irq, 0x08, 0x8E);
	unmask_irq(PIT_IRQ);
	pit_oneshot.flags |= TIMER_ENABLED;

	return 0;
}

static int pit_oneshot_disable(void)
{
	mask_irq(PIT_IRQ);
	idt_set(IRQ_TO_VECTOR(PIT_IRQ), NULL, 0, 0);
	pit_data_port_write(PIT_CHANNEL_0_PORT, 0);
	pit_oneshot.flags &= ~TIMER_ENABLED;

	return 0;
}

static struct irq_timer pit_oneshot = {
	.schedule_irq   = pit_schedule_irq,
	.frequency      = PIT_OSC_FREQ,
	.max_ticks      = 0xFFFF,
	.flags          = 0,
	.enable         = pit_oneshot_enable,
	.disable        = pit_oneshot_disable,
	.name           = "pit_oneshot"
};

void pit_oneshot_register(void)
{
	set_irq_timer(&pit_oneshot);
}


/* PIT waiting for early boot timing */

static volatile int pit_wait_complete;

static void pit_wait_handler(__unused void *device)
{
	pit_wait_complete = 1;
}

/* pit_wait_setup: configure the PIT for pit_wait use */
int pit_wait_setup(void)
{
	if (request_fixed_irq(PIT_IRQ, &pit, pit_wait_handler) != 0)
		return 1;

	unmask_irq(PIT_IRQ);
	return 0;
}

void pit_wait_finish(void)
{
	release_irq(PIT_IRQ, &pit);
}

/*
 * pit_wait:
 * Use the PIT to busy-wait for the specified number of microseconds.
 */
void pit_wait(uint32_t us)
{
	uint32_t hz;
	uint16_t div;

	hz = USEC_PER_SEC / us;
	div = PIT_OSC_FREQ / hz;
	pit_wait_complete = 0;

	outb(PIT_COMMAND_PORT, PIT_CHANNEL_0 |
	                       PIT_ACCESS_MODE_LO_HI |
	                       PIT_MODE_TERMINAL);
	pit_data_port_write(PIT_CHANNEL_0_PORT, div);

	while (!pit_wait_complete)
		;
}
