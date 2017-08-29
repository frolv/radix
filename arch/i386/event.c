/*
 * arch/i386/event.c
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

#include <radix/asm/pic.h>
#include <radix/asm/regs.h>

/*
 * update_intctx:
 * Copy the necessary values from the specified interrupt context's
 * registers struct to their positions on the stack.
 */
static void update_intctx(struct interrupt_context *intctx)
{
	intctx->ip = intctx->regs.ip;
	intctx->cs = intctx->regs.cs;
	intctx->flags = intctx->regs.flags;
	intctx->sp = intctx->regs.sp;
	intctx->ss = intctx->regs.ss;
}

void arch_event_handler(struct interrupt_context *intctx)
{
	system_pic->eoi(0);
	update_intctx(intctx);
}
