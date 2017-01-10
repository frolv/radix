/*
 * arch/i386/vga.h
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

#ifndef ARCH_I386_VGA_H
#define ARCH_I386_VGA_H

#include <untitled/types.h>

#define VGA_WIDTH       80
#define VGA_HEIGHT      25

/* location of the VGA text mode buffer */
#define VGA_TEXT_BUFFER_ADDR 0xC00B8000

#define VGA_NORMAL	0x0
#define VGA_BOLD	0x8

enum vga_color {
	VGA_COLOR_BLACK,
	VGA_COLOR_BLUE,
	VGA_COLOR_GREEN,
	VGA_COLOR_CYAN,
	VGA_COLOR_RED,
	VGA_COLOR_MAGENTA,
	VGA_COLOR_BROWN,
	VGA_COLOR_WHITE
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg)
{
	return fg | bg << 4;
}

static inline uint16_t vga_entry(uint8_t c, uint8_t color)
{
	return (uint16_t)c | (uint16_t)color << 8;
}

#endif /* ARCH_I386_VGA_H */
