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

#include <radix/irq.h>
#include <radix/kernel.h>
#include <radix/tty.h>
#include <rlibc/stdio.h>

/*
 * panic:
 * Print error message and halt the system.
 * This function never returns.
 */
__noreturn void panic(const char *err, ...)
{
	va_list ap;

	/* disable interrupts */
	irq_disable();

	printf("kernel panic: ");
	va_start(ap, err);
	vprintf(err, ap);
	va_end(ap);
	tty_flush();

	DIE();
	__builtin_unreachable();
}
