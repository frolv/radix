/*
 * kernel/console.c
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

#include <radix/assert.h>
#include <radix/console.h>

#include <string.h>

struct console *active_console = NULL;
static struct list console_list = LIST_INIT(console_list);

void console_register(struct console *console,
                      const char *name,
                      struct consfn *console_func,
                      int active)
{
    assert(console);

    strlcpy(console->name, name, sizeof console->name);
    console->actions = console_func;

    list_ins(&console_list, &console->list);

    console->actions->init(console);
    if (active)
        active_console = console;
}
