/*
 * drivers/video/console/vgatext.c
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
#include <radix/console.h>
#include <radix/io.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/mm.h>

#include <rlibc/string.h>

#define VGATEXT_WIDTH   80
#define VGATEXT_HEIGHT  25
#define VGATEXT_PHYS    0x000B8000
#define VGATEXT_BUFFER  (phys_to_virt(VGATEXT_PHYS))
#define VGATEXT_TABSTOP 2
#define VGATEXT_NORMAL  0
#define VGATEXT_BOLD    (1 << 3)

#define VGA_MISC_OUTPUT 0x3C2

static __always_inline uint8_t vgatext_entry_color(uint8_t fg, uint8_t bg)
{
	return fg | bg << 4;
}

static __always_inline uint16_t vgatext_entry(uint8_t c, uint8_t color)
{
	return (uint16_t)c | (uint16_t)color << 8;
}

static struct console vgatext_console;
static struct consfn vgatext_fn;

void vgatext_register(void)
{
	console_register(&vgatext_console, "vgatext", &vgatext_fn, 1);
	klog_set_console(&vgatext_console);
}

/* vgatext_clear: clear the VGA text buffer */
static int vgatext_clear(struct console *c)
{
	int x, y, ind;
	uint16_t *screenbuf = c->screenbuf;

	mutex_lock(&c->lock);
	for (y = 0; y < VGATEXT_HEIGHT; ++y) {
		for (x = 0; x < VGATEXT_WIDTH; ++x) {
			ind = y * VGATEXT_WIDTH + x;
			screenbuf[ind] = vgatext_entry(' ', c->color);
		}
	}

	c->cursor_x = 0;
	c->cursor_y = 0;
	mutex_unlock(&c->lock);

	return 0;
}

static int vgatext_init(struct console *c)
{
	uint8_t val;

	c->cols = VGATEXT_WIDTH;
	c->rows = VGATEXT_HEIGHT;
	c->cursor_x = 0;
	c->cursor_y = 0;
	c->screenbuf = (void *)VGATEXT_BUFFER;
	c->screenbuf_size = c->rows * c->cols * sizeof (uint16_t);
	c->fg_color = CON_COLOR_WHITE;
	c->bg_color = CON_COLOR_BLACK;
	c->default_color = vgatext_entry_color(c->fg_color, c->bg_color);
	c->color = c->default_color;
	mutex_init(&c->lock);

	/* set bit 0 of the misc output register to map port 0x3D4 */
	val = inb(VGA_MISC_OUTPUT);
	outb(VGA_MISC_OUTPUT, val | 1);

	return vgatext_clear(c);
}

/* vgatext_put: write ch to position x, y of vga text buffer */
static __always_inline void vgatext_put(struct console *c, int ch, int x, int y)
{
	uint16_t *screenbuf = c->screenbuf;

	screenbuf[y * VGATEXT_WIDTH + x] = vgatext_entry(ch, c->color);
}

/* vgatext_nextrow: advance to the next row, "scrolling" if necessary */
static void vgatext_nextrow(struct console *c)
{
	int x;

	c->cursor_x = 0;
	if (c->cursor_y == VGATEXT_HEIGHT - 1) {
		/* move each row up by one, discarding the first */
		memmove(c->screenbuf, c->screenbuf + VGATEXT_WIDTH,
		        c->cursor_y * VGATEXT_WIDTH * sizeof (uint16_t));
		/* clear the final row */
		for (x = 0; x < VGATEXT_WIDTH; ++x)
			vgatext_put(c, ' ', x, c->cursor_y);
	} else {
		++c->cursor_y;
	}
}

/* vgatext_putchar: write ch to current vga position */
static __always_inline void vgatext_putchar(struct console *c, int ch)
{
	vgatext_put(c, ch, c->cursor_x, c->cursor_y);
	if (++c->cursor_x == VGATEXT_WIDTH)
		vgatext_nextrow(c);
}

static void vgatext_update_cursor(int x, int y)
{
	int pos = y * VGATEXT_WIDTH + x;

	outb(0x3D4, 14);
	outb(0x3D5, (pos >> 8) & 0xFF);
	outb(0x3D4, 15);
	outb(0x3D5, pos & 0xFF);
}

/*
 * vgatext_write:
 * Write `n` characters from `buf` to the VGA text buffer.
 */
static int vgatext_write(struct console *c, const char *buf, size_t n)
{
	int written;

	written = 0;
	mutex_lock(&c->lock);
	while (n--) {
		switch (*buf) {
		case '\b':
			if (c->cursor_x == 0) {
				c->cursor_x = VGATEXT_WIDTH - 1;
				--c->cursor_y;
			} else {
				--c->cursor_x;
			}
			break;
		case '\n':
			vgatext_nextrow(c);
			break;
		case '\t':
			do {
				vgatext_putchar(c, ' ');
			} while (!ALIGNED(c->cursor_x, VGATEXT_TABSTOP));
			break;
		default:
			vgatext_putchar(c, *buf);
			break;
		}
		++written;
		++buf;
	}
	mutex_unlock(&c->lock);
	vgatext_update_cursor(c->cursor_x, c->cursor_y);

	return written;
}

/* vgatext_putc: write character `ch` to the VGA text buffer */
static int vgatext_putc(struct console *c, int ch)
{
	char put = ch;

	return vgatext_write(c, &put, 1);
}

/*
 * vgatext_set_color:
 * Set VGA foreground colour to `fg` and background colour to `bg`.
 */
static int vgatext_set_color(struct console *c, int fg, int bg)
{
	mutex_lock(&c->lock);
	if (fg != -1)
		c->fg_color = fg;
	if (bg != -1)
		c->bg_color = bg;

	c->color = vgatext_entry_color(c->fg_color, c->bg_color);
	mutex_unlock(&c->lock);

	return 0;
}

static int vgatext_move_cursor(struct console *c, int x, int y)
{
	c->cursor_x = x;
	c->cursor_y = y;
	vgatext_update_cursor(c->cursor_x, c->cursor_y);

	return 0;
}

/* vgatext_dummy: dummy function for unimplemented operations */
int vgatext_dummy()
{
	return 0;
}

static struct consfn vgatext_fn = {
	.init           = vgatext_init,
	.putc           = vgatext_putc,
	.write          = vgatext_write,
	.clear          = vgatext_clear,
	.set_color      = vgatext_set_color,
	.move_cursor    = vgatext_move_cursor,
	.destroy        = vgatext_dummy
};
