/*
 * arch/i386/include/radix/asm/irq.h
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

#ifndef ARCH_I386_RADIX_VECTORS_H
#define ARCH_I386_RADIX_VECTORS_H

// Interrupt vector setup for an x86 system.

#define X86_NUM_INTERRUPT_VECTORS 256
#define X86_NUM_EXCEPTION_VECTORS 32
#define X86_NUM_RESERVED_VECTORS  64
#define X86_NUM_ASSIGNABLE_VECTORS                           \
    (X86_NUM_INTERRUPT_VECTORS - X86_NUM_EXCEPTION_VECTORS - \
     X86_NUM_RESERVED_VECTORS)

#define ISA_IRQ_COUNT 16
#define IRQ_BASE      0x20

// Vectors 0 through 31 are reserved for exceptions.

#define X86_FIRST_EXCEPTION_VECTOR 0
#define X86_LAST_EXCEPTION_VECTOR  (IRQ_BASE - 1)

#define X86_EXCEPTION_DE 0x00
#define X86_EXCEPTION_DB 0x01
#define X86_EXCEPTION_BP 0x03
#define X86_EXCEPTION_OF 0x04
#define X86_EXCEPTION_BR 0x05
#define X86_EXCEPTION_UD 0x06
#define X86_EXCEPTION_NM 0x07
#define X86_EXCEPTION_DF 0x08
#define X86_EXCEPTION_CP 0x09
#define X86_EXCEPTION_TS 0x0A
#define X86_EXCEPTION_NP 0x0B
#define X86_EXCEPTION_SS 0x0C
#define X86_EXCEPTION_GP 0x0D
#define X86_EXCEPTION_PF 0x0E
#define X86_EXCEPTION_MF 0x10
#define X86_EXCEPTION_AC 0x11
#define X86_EXCEPTION_MC 0x12
#define X86_EXCEPTION_XM 0x13
#define X86_EXCEPTION_VE 0x14
#define X86_EXCEPTION_SX 0x1E

// IRQs start following the reserved exception vectors. The first 16 (vectors
// 32 through 47) are ISA IRQs. The system PIC may additionally have more IRQs
// on top of these. (For example, IOAPICs often have 24 in total.) The ISA and
// PIC vectors can be requested explicitly.
//
// After the PIC IRQs, the majority of interrupt vectors fall into an assignable
// pool. Drivers can request a vector when they are loaded and the system will
// find one from this pool.

#define X86_FIRST_ASSIGNABLE_VECTOR IRQ_BASE
#define X86_LAST_ASSIGNABLE_VECTOR  (X86_FIRST_RESERVED_VECTOR - 1)

// Finally, the system reserves the last 64 vectors (0xc0 - 0xff) for its own
// use.

#define X86_FIRST_RESERVED_VECTOR \
    (X86_NUM_INTERRUPT_VECTORS - X86_NUM_RESERVED_VECTORS)

#define IPI_VEC_PANIC         0xC0
#define IPI_VEC_TLB_SHOOTDOWN 0xC1
#define IPI_VEC_TIMER_ACTION  0xC2
#define IPI_VEC_SCHED_WAKE    0xC3

// x86 syscall interrupt uses vector 222 (0xde).
#define VEC_SYSCALL 0xDE

#define APIC_VEC_TIMER    0xE0
#define APIC_VEC_NMI      0xE1
#define APIC_VEC_SMI      0xE2
#define APIC_VEC_EXTINT   0xE3
#define APIC_VEC_ERROR    0xE4
#define APIC_VEC_THERMAL  0xE5
#define APIC_VEC_CMCI     0xE6
#define APIC_VEC_SPURIOUS 0xE7

#endif  // ARCH_I386_RADIX_VECTORS_H
