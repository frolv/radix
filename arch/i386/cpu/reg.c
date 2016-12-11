/*
 * arch/i386/cpu/reg.c
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

#include <untitled/mm_types.h>
#include <untitled/sys.h>

extern void kthread_exit(void);

#define PUSHL(s, x) \
	do { \
		(s) -= sizeof (uint32_t);\
		*((uint32_t *)s) = (x);\
	} while (0)

#define KERNEL_CS 0x08
#define KERNEL_DS 0x10

/*
 * Setup stack and registers for a kthread to execute function func
 * with argument arg.
 */
void kthread_reg_setup(struct regs *r, addr_t stack, addr_t func, addr_t arg)
{
	/* push argument, return address and stored ebp */
	PUSHL(stack, arg);
	PUSHL(stack, (addr_t)kthread_exit);
	PUSHL(stack, 0);

	r->bp = stack;
	r->sp = stack;
	r->ip = (addr_t)func;

	r->gs = KERNEL_DS;
	r->fs = KERNEL_DS;
	r->es = KERNEL_DS;
	r->ds = KERNEL_DS;
	r->ss = KERNEL_DS;

	r->cs = KERNEL_CS;
}
