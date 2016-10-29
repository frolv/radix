/*
 * arch/i386/include/untitled/sys.h
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

#ifndef UNTITLED_ARCH_I386_UNTITLED_SYS_H
#define UNTITLED_ARCH_I386_UNTITLED_SYS_H

#include <stdint.h>

struct regs {
	/* gprs */
	uint32_t di;
	uint32_t si;
	uint32_t bp;
	uint32_t sp;
	uint32_t bx;
	uint32_t dx;
	uint32_t cx;
	uint32_t ax;

	/* segment registers */
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;

	uint32_t intno;
	uint32_t errno;
};

#endif /* UNTITLED_ARCH_I386_UNTITLED_SYS_H */
