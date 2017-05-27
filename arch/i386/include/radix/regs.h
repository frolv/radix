/*
 * arch/i386/include/radix/sys.h
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

#ifndef ARCH_I386_RADIX_SYS_H
#define ARCH_I386_RADIX_SYS_H

#include <radix/types.h>
#include <radix/mm_types.h>

struct interrupt_regs {
	uint32_t di;
	uint32_t si;
	uint32_t bp;
	uint32_t bx;
	uint32_t dx;
	uint32_t cx;
	uint32_t ax;

	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;

	int32_t intno;
	int32_t errno;

	uint32_t ip;
	uint32_t cs;
	uint32_t flags;
	uint32_t sp;
	uint32_t ss;
};

struct regs {
	/* gprs */
	uint32_t di;
	uint32_t si;
	uint32_t sp;
	uint32_t bp;
	uint32_t bx;
	uint32_t dx;
	uint32_t cx;
	uint32_t ax;

	/* segment registers */
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;
	uint32_t cs;
	uint32_t ss;

	uint32_t ip;
	uint32_t flags;
};

void save_registers(struct interrupt_regs *ir, struct regs *r);
void load_registers(struct interrupt_regs *ir, struct regs *r);

void kthread_reg_setup(struct regs *r, addr_t stack, addr_t func, addr_t arg);

#endif /* ARCH_I386_RADIX_SYS_H */
