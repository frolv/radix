/*
 * kernel/panic.c
 * Copyright (C) 2016-2018 Alexei Frolov
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
#include <radix/stacktrace.h>

#include <rlibc/stdio.h>
#include <rlibc/string.h>

#define PANIC           "kernel panic: "
#define PANIC_BUFSIZE   8192
#define PANIC_TRACESIZE (PANIC_BUFSIZE - 1024)

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
	char buf[PANIC_BUFSIZE];
	va_list ap;
	char *s;

	irq_disable();

	send_panic_ipi();

	s = buf;

#ifdef DEBUG_STACKTRACE
	s += stack_trace(buf, PANIC_TRACESIZE);
#endif

	/* TODO: register dump */

	strcpy(s, PANIC);
	s += sizeof PANIC - 1;

	va_start(ap, err);
	s += vsprintf(s, err, ap);
	va_end(ap);

	raw_write(buf, s - buf);

	DIE();
}
