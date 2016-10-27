/*
 * arch/i386/tty.c
 * Copyright (C) 2016 Alexei Frolov
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

#include <string.h>
#include <os/tty.h>

#include "vga.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* location of the VGA text mode buffer */
#define VGA_TEXT_BUFFER_ADDR 0xb8000

static size_t tty_row;
static size_t tty_col;
static uint8_t tty_color;
static uint16_t *tty_buf;

/* tty_init: initialize tty variables and populate vga buffer */
void tty_init(void)
{
	size_t x, y, ind;

	tty_row = 0;
	tty_col = 0;
	tty_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	tty_buf = (uint16_t *)VGA_TEXT_BUFFER_ADDR;

	for (y = 0; y < VGA_HEIGHT; ++y) {
		for (x = 0; x < VGA_WIDTH; ++x) {
			ind = y * VGA_WIDTH + x;
			tty_buf[ind] = vga_entry(' ', tty_color);
		}
	}
}

static void tty_nextrow(void);
static void tty_put(int c, uint8_t color, size_t x, size_t y);

/* tty_putchar: write character c at current tty position, and increment pos */
void tty_putchar(int c)
{
	if (c == '\n') {
		tty_nextrow();
		return;
	} else if (c == '\t') {
		while ((++tty_col) % TTY_TAB_STOP != 0) {
			if (tty_col == VGA_WIDTH)
				tty_nextrow();
		}
		return;
	}

	tty_put(c, tty_color, tty_col, tty_row);
	if (++tty_col == VGA_WIDTH)
		tty_nextrow();
}

/* tty_write: write size bytes of string data to the tty */
void tty_write(const char *data, size_t size)
{
	while (size--)
		tty_putchar(*data++);
}

/* tty_nextrow: advance to the next row, "scrolling" if necessary */
static void tty_nextrow(void)
{
	size_t x, dst;

	tty_col = 0;
	if (tty_row == VGA_HEIGHT - 1) {
		/* move each row up by one, discarding the first */
		memmove(tty_buf, tty_buf + VGA_WIDTH,
				 tty_row * VGA_WIDTH * sizeof(uint16_t));
		/* clear the final row */
		for (x = 0; x < VGA_WIDTH; ++x) {
			dst = tty_row * VGA_WIDTH + x;
			tty_buf[dst] = vga_entry(' ', tty_color);
		}
	} else {
		++tty_row;
	}
}

/* tty_put: write c to position x, y */
static void tty_put(int c, uint8_t color, size_t x, size_t y)
{
	size_t ind;

	ind = y * VGA_WIDTH + x;
	tty_buf[ind] = vga_entry(c, color);
}
