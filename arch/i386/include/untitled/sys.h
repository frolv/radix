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

struct regs {
	/* gprs */
	unsigned int di;
	unsigned int si;
	unsigned int bp;
	unsigned int sp;
	unsigned int bx;
	unsigned int dx;
	unsigned int cx;
	unsigned int ax;

	/* segment registers */
	unsigned int gs;
	unsigned int fs;
	unsigned int es;
	unsigned int ds;

	unsigned int intno;
	unsigned int errno;
};

#endif /* UNTITLED_ARCH_I386_UNTITLED_SYS_H */
