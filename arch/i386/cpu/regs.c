/*
 * arch/i386/cpu/reg.c
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

#include <radix/asm/gdt.h>
#include <radix/asm/regs.h>
#include <radix/cpu.h>
#include <radix/kthread.h>
#include <radix/mm_types.h>

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

    r->gs = GDT_OFFSET(GDT_GS);
    r->fs = GDT_OFFSET(GDT_FS);
    r->es = GDT_OFFSET(GDT_KERNEL_DATA);
    r->ds = GDT_OFFSET(GDT_KERNEL_DATA);
    r->ss = GDT_OFFSET(GDT_KERNEL_DATA);

    r->cs = GDT_OFFSET(GDT_KERNEL_CODE);
    r->flags = EFLAGS_IF | EFLAGS_ID;
}
