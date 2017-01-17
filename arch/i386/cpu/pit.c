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

#include <untitled/asm/io.h>
#include <untitled/types.h>

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

void pit_init(void)
{
	const int freq = 1000;

	pit_start(pit_divisor(freq));
}
