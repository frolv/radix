/*
 * arch/i386/irq/ipi.c
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

#include <radix/asm/idt.h>
#include <radix/asm/pic.h>

#include <radix/ipi.h>
#include <radix/smp.h>
#include <radix/timer.h>

void panic_shutdown(void);
void tlb_shootdown(void);
void timer_action(void);

void arch_ipi_init(void)
{
	idt_set(IPI_VEC_PANIC, panic_shutdown, 0x08, 0x8E);
	idt_set(IPI_VEC_TLB_SHOOTDOWN, tlb_shootdown, 0x08, 0x8E);
	idt_set(IPI_VEC_TIMER_ACTION, timer_action, 0x08, 0x8E);
}

void i386_send_panic_ipi(void)
{
	system_pic->send_ipi(IPI_VEC_PANIC, CPUMASK_ALL_OTHER);
}

void i386_send_timer_ipi(void)
{
	system_pic->send_ipi(IPI_VEC_TIMER_ACTION, CPUMASK_ALL_OTHER);
}

void timer_action_handler(void)
{
	system_pic->eoi(IPI_VEC_TIMER_ACTION);
	handle_timer_action();
}
