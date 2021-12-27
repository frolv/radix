/*
 * arch/i386/exceptions.c
 * Copyright (C) 2021 Alexei Frolov
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

#include "exceptions.h"

#include <radix/asm/gdt.h>
#include <radix/asm/regs.h>
#include <radix/compiler.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/smp.h>
#include <radix/task.h>

#include <stdbool.h>

static bool is_user_mode_interrupt(const struct interrupt_context *intctx)
{
    return intctx->regs.cs == (GDT_OFFSET(GDT_USER_CODE) | 0x3);
}

void div_error_handler(const struct interrupt_context *intctx)
{
    if (is_user_mode_interrupt(intctx)) {
        // TODO(frolv): Terminate process.
        panic("division error in user process %d", current_task()->pid);
    } else {
        panic("division error at eip %p", intctx->regs.ip);
    }
}

void debug_handler(const struct interrupt_context *intctx)
{
    // TODO(frolv): Handle this.
    klog(KLOG_WARNING,
         "Debug exception at eip %p on cpu%d",
         intctx->regs.ip,
         processor_id());
}

void double_fault_handler(const struct interrupt_context *intctx)
{
    // TODO(frolv): Handle this.
    panic("Double fault exception at eip %p", intctx->regs.ip);
}

void gpf_handler(const struct interrupt_context *intctx, uint32_t error)
{
    // TODO(frolv): This just provides debug information for now.
    // Do something smarter here.
    uint32_t tbl = (error >> 1) & 0x3;
    uint32_t idx = (error >> 3) & 0x1fff;

    panic("General protection fault! eip: %p, table: %u, index: %u",
          intctx->regs.ip,
          tbl,
          idx);
}
