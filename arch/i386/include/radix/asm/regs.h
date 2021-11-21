/*
 * arch/i386/include/radix/asm/regs.h
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

#ifndef ARCH_I386_RADIX_REGS_H
#define ARCH_I386_RADIX_REGS_H

#include <radix/mm_types.h>
#include <radix/types.h>

// Registers in an x86 system. Must be kept in sync with regs_asm.h and
// arch/i386/irq/isr.S.
struct regs {
    // GPRs
    uint32_t di;
    uint32_t si;
    uint32_t sp;
    uint32_t bp;
    uint32_t bx;
    uint32_t dx;
    uint32_t cx;
    uint32_t ax;

    // Segment registers
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t cs;
    uint32_t ss;

    uint32_t ip;
    uint32_t flags;

    // TODO(frolv): FPU, SSE, etc.
};

// The layout of the stack during an interrupt, as set up by _interrupt_common
// in arch/i386/irq/isr.S.
struct interrupt_context {
    struct regs regs;
    uint32_t handler;
    uint32_t code;
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t sp;
    uint32_t ss;
};

void kthread_reg_setup(struct regs *r, addr_t stack, addr_t func, addr_t arg);

#endif  // ARCH_I386_RADIX_REGS_H
