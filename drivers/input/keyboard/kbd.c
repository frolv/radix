/*
 * drivers/input/keyboard/kbd.c
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

#include <radix/asm/io.h>
#include <radix/irq.h>
#include <radix/regs.h>

/* this will be converted to a kernel module eventually */

void kbd_handler(struct regs *r)
{
	uint8_t c;

	c = inb(0x60);

	(void)c;
	(void)r;
}

void kbd_install(void)
{
	irq_install(1, kbd_handler);
}
