/*
 * arch/i386/timers/rtc.c
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
#include <radix/timer.h>

/*
 * The real-time clock (RTC) is the lowest common denominator timer for the
 * x86 architecture. A counter is emulated through RTC interrupts, running
 * at a frequency of 2048Hz. This provides a measly 488us resolution timer.
 * Although the RTC is capable of running at higher frequencies, it requires
 * two legacy ISA port accesses within its IRQ handler, each costing
 * approximately 1us. At a frequency of 2048Hz, this wastes 4ms per second.
 * A higher frequency would make these port accesses far too expensive.
 * Due to its poor precision and I/O limitations, the RTC has the lowest
 * rating of any x86 timer.
 */

#define RTC_PORT_REG  0x70
#define RTC_PORT_WIN  0x71
#define RTC_REG_A     0x0A
#define RTC_REG_B     0x0B
#define RTC_REG_C     0x0C

#define RTC_ENABLE    (1 << 6)

#define RTC_FREQUENCY 2048
#define RTC_SHIFT     2
#define RTC_MULT      1953125

#define RTC_IRQ       8

static uint64_t rtc_ticks = 0;
static struct timer rtc;

static uint64_t rtc_read(void)
{
	return rtc_ticks;
}

static void rtc_tick_handler(__unused void *device)
{
	++rtc_ticks;
	/* read register C to reset IRQ */
	outb(RTC_PORT_REG, RTC_REG_C);
	inb(RTC_PORT_WIN);
}

static uint8_t __rtc_reg_read(int reg)
{
	outb(RTC_PORT_REG, reg);
	return inb(RTC_PORT_WIN);
}

static void __rtc_reg_write(int reg, uint8_t val)
{
	outb(RTC_PORT_REG, reg);
	outb(RTC_PORT_WIN, val);
}

/*
 * __rtc_modify_reg:
 * Modify the value of the specified RTC register by clearing and setting
 * the given bits.
 */
static void __rtc_modify_reg(int reg, unsigned int clear, unsigned int set)
{
	int val;

	irq_disable();
	val = __rtc_reg_read(reg);
	val &= ~clear;
	val |= set;
	__rtc_reg_write(reg, val);
	irq_enable();
}

static int rtc_enable(void)
{
	/*
	 * The low four bits of RTC_REG_A specify the frequency divider,
	 * where RTC frequency = 32768 >> (RTC_REG_A[0:3] - 1).
	 * For our target frequency of 2048Hz, we set the divider to 5.
	 */
	__rtc_modify_reg(RTC_REG_A, 0xF, 5);
	if (request_fixed_irq(RTC_IRQ, &rtc, rtc_tick_handler) != 0)
		return 1;

	rtc.flags |= TIMER_ENABLED;
	return 0;
}

static int rtc_disable(void)
{
	release_irq(RTC_IRQ, &rtc);
	rtc.flags &= ~TIMER_ENABLED;
	return 0;
}

static void rtc_start(void)
{
	__rtc_modify_reg(RTC_REG_B, 0, RTC_ENABLE);
	rtc.flags |= TIMER_RUNNING;
	unmask_irq(RTC_IRQ);
}

static void rtc_stop(void)
{
	__rtc_modify_reg(RTC_REG_B, RTC_ENABLE, 0);
	rtc.flags &= ~TIMER_RUNNING;
	mask_irq(RTC_IRQ);
}

static struct timer rtc = {
	.read           = rtc_read,
	.mult           = RTC_MULT,
	.shift          = RTC_SHIFT,
	.frequency      = RTC_FREQUENCY,
	.start          = rtc_start,
	.stop           = rtc_stop,
	.enable         = rtc_enable,
	.disable        = rtc_disable,
	.flags          = TIMER_EMULATED,
	.name           = "rtc",
	.rating         = 1,
	.timer_list     = LIST_INIT(rtc.timer_list)
};

void rtc_register(void)
{
	timer_register(&rtc);
}
