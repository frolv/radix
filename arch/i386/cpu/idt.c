/*
 * arch/i386/cpu/idt.c
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

#include <radix/asm/idt.h>
#include <radix/irq.h>
#include <radix/kernel.h>
#include <radix/percpu.h>

#include "exceptions.h"
#include "pic8259.h"

static uint64_t idt[IDT_ENTRIES];

extern uint64_t irq_fn[NUM_INTERRUPT_VECTORS - NUM_EXCEPTION_VECTORS];
extern void idt_load(void *base, size_t s);

static uint64_t idt_pack(uintptr_t intfn, uint16_t selector, uint8_t gate)
{
    uint64_t ret;

    ret = intfn & 0xFFFF0000;
    ret |= ((uint16_t)gate << 8) & 0xFF00;

    ret <<= 32;
    ret |= (uint32_t)selector << 16;
    ret |= intfn & 0x0000FFFF;

    return ret;
}

// Sets a single interrupt vector in the IDT.
void idt_set(size_t vector, void (*intfn)(void), uint16_t selector, uint8_t gate)
{
    idt[vector] = idt_pack((uintptr_t)intfn, selector, gate);
}

void idt_init(void) { idt_load(idt, sizeof idt); }

// Configures and loads an interrupt descriptor table containing interrupt
// handlers for CPU exceptions.
void idt_init_early(void)
{
    size_t i;

    idt_set(X86_EXCEPTION_DE,
            div_error,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_DB,
            debug,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_BP,
            breakpoint,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_OF,
            overflow,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_BR,
            bound_range,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_UD,
            invalid_opcode,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_NM,
            device_not_available,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_DF,
            double_fault,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_CP,
            coprocessor_segment,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_TS,
            invalid_tss,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_NP,
            segment_not_present,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_SS,
            stack_segment,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_GP,
            general_protection_fault,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_PF,
            page_fault,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_MF,
            x87_floating_point,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_AC,
            alignment_check,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_MC,
            machine_check,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_XM,
            simd_floating_point,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_VE,
            virtualization_exception,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);
    idt_set(X86_EXCEPTION_SX,
            security_exception,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_32BIT_TRAP_GATE);

    for (i = 0; i < ARRAY_SIZE(irq_fn); ++i) {
        idt[i + IRQ_BASE] = idt_pack((uintptr_t)&irq_fn[i],
                                     GDT_OFFSET(GDT_KERNEL_CODE),
                                     IDT_32BIT_INTERRUPT_GATE);
    }

    pic8259_init();
    pic8259_disable();

    idt_load(idt, sizeof idt);
}
