/*
 * arch/i386/include/radix/asm/pic.h
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

#ifndef ARCH_I386_RADIX_PIC_H
#define ARCH_I386_RADIX_PIC_H

#include <radix/cpumask.h>

struct pic {
	char            name[16];
	unsigned int    irq_count;
	void            (*eoi)(unsigned int);
	void            (*mask)(unsigned int);
	void            (*unmask)(unsigned int);
	int             (*send_ipi)(unsigned int, cpumask_t);
};

extern struct pic *system_pic;

#endif /* ARCH_I386_RADIX_PIC_H */
