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

#include <untitled/cpu.h>
#include <untitled/kthread.h>
#include <untitled/mm_types.h>
#include <untitled/sys.h>

#define KERNEL_CS 0x08
#define KERNEL_DS 0x10

/* save registers from interrupt into r */
void save_registers(struct interrupt_regs *ir, struct regs *r)
{
	r->di = ir->di;
	r->si = ir->si;
	r->sp = ir->sp;
	r->bp = ir->bp;
	r->bx = ir->bx;
	r->dx = ir->dx;
	r->cx = ir->cx;
	r->ax = ir->ax;

	r->gs = ir->gs;
	r->fs = ir->fs;
	r->es = ir->es;
	r->ds = ir->ds;
	r->cs = ir->cs;
	r->ss = ir->ss;

	r->ip = ir->ip;
	r->flags = ir->flags;
}

/* reload registers from r into interrupt form */
void load_registers(struct interrupt_regs *ir, struct regs *r)
{
	ir->di = r->di;
	ir->si = r->si;
	ir->sp = r->sp;
	ir->bp = r->bp;
	ir->bx = r->bx;
	ir->dx = r->dx;
	ir->cx = r->cx;
	ir->ax = r->ax;

	ir->gs = r->gs;
	ir->fs = r->fs;
	ir->es = r->es;
	ir->ds = r->ds;
	ir->cs = r->cs;
	ir->ss = r->ss;

	ir->ip = r->ip;
	ir->flags = r->flags;
}

/*
 * Setup stack and registers for a kthread to execute function func
 * with argument arg.
 */
void kthread_reg_setup(struct regs *r, addr_t stack, addr_t func, addr_t arg)
{
	uint32_t *s;

	s = (uint32_t *)stack;

	s[-1] = 0;
	s[-2] = 0;
	s[-3] = 0;
	/* argument and return address */
	s[-4] = arg;
	s[-5] = (addr_t)kthread_exit;

	r->bp = (addr_t)(s - 3);
	r->sp = (addr_t)(s - 5);
	r->ip = (addr_t)func;

	r->gs = KERNEL_DS;
	r->fs = KERNEL_DS;
	r->es = KERNEL_DS;
	r->ds = KERNEL_DS;
	r->ss = KERNEL_DS;

	r->cs = KERNEL_CS;
	r->flags = EFLAGS_IF | EFLAGS_ID;
}
