/*
 * arch/i386/cpu/pit.c
 * Copyright (C) 2016-2017 Alexei Frolov
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

#include <radix/io.h>
#include <radix/irq.h>
#include <radix/sched.h>
#include <radix/regs.h>
#include <radix/types.h>

#include <rlibc/string.h>

#define PIT_0   0x40
#define PIT_1   0x41
#define PIT_2   0x42
#define PIT_CMD 0x43

#define PIT_OSC_FREQ 1193182

static __always_inline uint16_t pit_divisor(int freq)
{
	return PIT_OSC_FREQ / freq;
}

static void pit_start(uint16_t divisor)
{
	outb(PIT_CMD, 0x36);
	outb(PIT_0, divisor & 0xFF);
	outb(PIT_0, (divisor >> 8) & 0xFF);
}

/*
 * pit_irq0:
 * Timer IRQ handler when PIT is used as system timer.
 */
void pit_irq0(struct regs *r)
{
	memcpy(&current_task->regs, r, sizeof *r);
	schedule(0);
	memcpy(r, &current_task->regs, sizeof *r);
}

void pit_init(void)
{
	/* Run the PIT at 1000 Hz */
	const int freq = 1000;

	pit_start(pit_divisor(freq));
	irq_install(TIMER_IRQ, pit_irq0);
}
