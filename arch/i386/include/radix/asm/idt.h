/*
 * arch/i386/include/radix/asm/idt.h
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

#ifndef ARCH_I386_RADIX_IDT_H
#define ARCH_I386_RADIX_IDT_H

#include <radix/asm/gdt.h>
#include <radix/irq.h>

#include <stddef.h>
#include <stdint.h>

void idt_init_early(void);
void idt_init(void);

// Sets a single interrupt vector in the IDT.
void idt_set(size_t vector,
             void (*intfn)(void),
             uint16_t selector,
             uint8_t gate);

// Clears an interrupt vector.
static inline void idt_unset(size_t vector) { idt_set(vector, NULL, 0, 0); }

#define IDT_GATE_TASK 0x5
#define IDT_GATE_INT  0xe
#define IDT_GATE_TRAP 0xf
#define IDT_DPL(x)    ((uint8_t)(x & 0x3) << 5)
#define IDT_PRESENT   (1u << 7)

#define IDT_32BIT_TASK_GATE      (IDT_GATE_TASK | IDT_DPL(0) | IDT_PRESENT)
#define IDT_32BIT_INTERRUPT_GATE (IDT_GATE_INT | IDT_DPL(0) | IDT_PRESENT)
#define IDT_32BIT_TRAP_GATE      (IDT_GATE_TRAP | IDT_DPL(0) | IDT_PRESENT)

#endif  // ARCH_I386_RADIX_IDT_H
