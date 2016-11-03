/*
 * kernel/kernel.c
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

#include <stdio.h>
#include <untitled/irq.h>

/* kernel entry point */
int kmain(void)
{
	printf("Kernel loaded\n");

	irq_enable();

	/* TEMP: until modules are implemented (so a while) */
	extern void kbd_install(void);
	kbd_install();
	printf("Welcome to UNTITLED!\n");
	printf("Press `q' for a kernel panic\n");

	while (1)
		;

	return 0;
}
