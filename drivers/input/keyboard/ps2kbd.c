/*
 * drivers/input/keyboard/ps2kbd.c
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

#include <radix/compiler.h>
#include <radix/io.h>
#include <radix/irq.h>
#include <radix/klog.h>

#define PS2_KEYBOARD_IRQ 1

static int kbdev;

void kbd_handler(__unused void *device)
{
	uint8_t c;

	c = inb(0x60);

	(void)c;
}

void kbd_install(void)
{
	if (request_fixed_irq(PS2_KEYBOARD_IRQ, &kbdev, kbd_handler) != 0) {
		klog(KLOG_ERROR, "failed to map PS2 keyboard to IRQ 1");
		return;
	}
}
