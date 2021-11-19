/*
 * include/radix/console.h
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

#ifndef RADIX_CONSOLE_H
#define RADIX_CONSOLE_H

#include <radix/list.h>
#include <radix/mutex.h>
#include <radix/types.h>

#define CON_NORMAL 0
#define CON_BOLD   (1 << 3)

enum console_color {
    CON_COLOR_BLACK,
    CON_COLOR_BLUE,
    CON_COLOR_GREEN,
    CON_COLOR_CYAN,
    CON_COLOR_RED,
    CON_COLOR_MAGENTA,
    CON_COLOR_BROWN,
    CON_COLOR_WHITE
};

struct console;

struct consfn {
    int (*init)(struct console *);
    int (*putc)(struct console *, int);
    int (*write)(struct console *, const char *, size_t);
    int (*clear)(struct console *);
    int (*set_color)(struct console *, int, int);
    int (*move_cursor)(struct console *, int, int);
    int (*destroy)(struct console *);
};

struct console {
    char name[16];
    int cols;
    int rows;
    int cursor_x;
    int cursor_y;
    void *screenbuf;
    size_t screenbuf_size;
    struct consfn *actions;
    uint8_t fg_color;
    uint8_t bg_color;
    uint8_t color;
    uint8_t default_color;
    struct mutex lock;
    struct list list;
};

extern struct console *active_console;

void console_register(struct console *console,
                      const char *name,
                      struct consfn *console_func,
                      int active);

#endif /* RADIX_CONSOLE_H */
