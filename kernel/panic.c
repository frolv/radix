/*
 * kernel/panic.c
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

#include <radix/atomic.h>
#include <radix/console.h>
#include <radix/list.h>
#include <radix/ipi.h>
#include <radix/irq.h>
#include <radix/kernel.h>

#include <rlibc/stdio.h>
#include <rlibc/string.h>

#define PANIC "kernel panic: "

static void raw_write(const char *s, size_t len)
{
	if (!active_console || !s)
		return;

	atomic_write(&active_console->lock.count, 0);
	list_init(&active_console->lock.queue);
	active_console->actions->write(active_console, s, len);
}

/*
 * panic:
 * Print error message and halt the system.
 */
__noreturn void panic(const char *err, ...)
{
	char buf[1024];
	va_list ap;
	char *s;

	irq_disable();

	/*
	 * TODO:
	 * stack trace
	 * register dump
	 */
	send_panic_ipi();

	strcpy(buf, PANIC);
	s = buf + sizeof PANIC - 1;

	va_start(ap, err);
	s += vsprintf(s, err, ap);
	va_end(ap);

	raw_write(buf, s - buf);

	DIE();
	__builtin_unreachable();
}
