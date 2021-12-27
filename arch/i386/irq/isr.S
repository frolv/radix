#
# arch/i386/irq/isr.S
# Copyright (C) 2021 Alexei Frolov
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

# Interrupt service routines for an i386 system.

#include <radix/asm/regs_asm.h>
#include <radix/asm/vectors.h>
#include <radix/assembler.h>

.macro UNHANDLED_EXCEPTION
	call exception_unhandled
	iret
.endm

# Per-CPU variable to track the number of unhandled exceptions.
DEFINE_PER_CPU(unhandled_exceptions)
	.long 0
DEFINE_PER_CPU_END()

DEFINE_PER_CPU(interrupt_depth)
	.long 0
DEFINE_PER_CPU_END()

# Creates a `struct regs` on the stack with the values of the current registers
# and the context pushed at the start of an interrupt.
#
# eax is set to the value of the original esp.
.macro PUSH_STRUCT_REGS pushed_bytes
	# Reserve space for eflags, eip, ss, and cs.
	subl $16, %esp

	push %ds
	push %es
	push %fs
	push %gs

	# Store the original esp value in eax.
	pushl %eax
	leal (16 + 16 + 4)(%esp), %eax

	pushl %ecx
	pushl %edx
	pushl %ebx
	pushl %ebp
	pushl %eax  # Original esp
	pushl %esi
	pushl %edi

	# Copy the values from the interrupt context into the struct.
	movl %ss, %edx
	movl %edx, REGS_SS(%esp)
	movl (72 + \pushed_bytes)(%esp), %edx
	movl %edx, REGS_FLAGS(%esp)
	movl (68 + \pushed_bytes)(%esp), %edx
	movl %edx, REGS_CS(%esp)
	movl (64 + \pushed_bytes)(%esp), %edx
	movl %edx, REGS_IP(%esp)
.endm

.macro POP_STRUCT_REGS
	popl %edi
	popl %esi
	# Skip the original esp.
	addl $4, %esp
	popl %ebp
	popl %ebx
	popl %edx
	popl %ecx
	popl %eax
	# Skip fs and gs -- these are specific to the CPU.
	addl $8, %esp
	pop %es
	pop %ds
	# Skip eip, cs, eip, and eflags, as they'll be handled by the iret.
	addl $16, %esp
.endm

#
# CPU exception handlers.
#

# Function for unimplemented exception handlers.
exception_unhandled:
	incl THIS_CPU_VAR(unhandled_exceptions)
	ret

BEGIN_FUNC(div_error)
	pushl $0
	pushl $div_error_handler
	jmp _interrupt_common
END_FUNC(div_error)

BEGIN_FUNC(debug)
	pushl $0
	pushl $debug_handler
	jmp _interrupt_common
END_FUNC(debug)

BEGIN_FUNC(breakpoint)
	UNHANDLED_EXCEPTION
END_FUNC(breakpoint)

BEGIN_FUNC(overflow)
	UNHANDLED_EXCEPTION
END_FUNC(overflow)

BEGIN_FUNC(bound_range)
	UNHANDLED_EXCEPTION
END_FUNC(bound_range)

BEGIN_FUNC(invalid_opcode)
	UNHANDLED_EXCEPTION
END_FUNC(invalid_opcode)

BEGIN_FUNC(device_not_available)
	UNHANDLED_EXCEPTION
END_FUNC(device_not_available)

BEGIN_FUNC(double_fault)
	pushl $double_fault_handler
	jmp _interrupt_common
END_FUNC(double_fault)

BEGIN_FUNC(coprocessor_segment)
	UNHANDLED_EXCEPTION
END_FUNC(coprocessor_segment)

BEGIN_FUNC(invalid_tss)
	UNHANDLED_EXCEPTION
END_FUNC(invalid_tss)

BEGIN_FUNC(segment_not_present)
	UNHANDLED_EXCEPTION
END_FUNC(segment_not_present)

BEGIN_FUNC(stack_segment)
	UNHANDLED_EXCEPTION
END_FUNC(stack_segment)

BEGIN_FUNC(general_protection_fault)
	pushl $gpf_handler
	jmp _interrupt_common
END_FUNC(general_protection_fault)

BEGIN_FUNC(page_fault)
	pushl $page_fault_handler
	jmp _interrupt_common
END_FUNC(page_fault)

BEGIN_FUNC(x87_floating_point)
	UNHANDLED_EXCEPTION
END_FUNC(x87_floating_point)

BEGIN_FUNC(alignment_check)
	UNHANDLED_EXCEPTION
END_FUNC(alignment_check)

BEGIN_FUNC(machine_check)
	UNHANDLED_EXCEPTION
END_FUNC(machine_check)

BEGIN_FUNC(simd_floating_point)
	UNHANDLED_EXCEPTION
END_FUNC(simd_floating_point)

BEGIN_FUNC(virtualization_exception)
	UNHANDLED_EXCEPTION
END_FUNC(virtualization_exception)

BEGIN_FUNC(security_exception)
	UNHANDLED_EXCEPTION
END_FUNC(security_exception)

#
# Custom kernel IRQ vectors.
#

BEGIN_FUNC(event_irq)
	push $(APIC_VEC_TIMER)
	push $arch_event_handler
	jmp _interrupt_common
END_FUNC(event_irq)

BEGIN_FUNC(lapic_error)
	push $(APIC_VEC_ERROR)
	pushl $lapic_error_handler
	jmp _interrupt_common
END_FUNC(lapic_error)

BEGIN_FUNC(sched_wake)
	push $(IPI_VEC_SCHED_WAKE)
	push $sched_wake_handler
	jmp _interrupt_common
END_FUNC(sched_wake)

BEGIN_FUNC(timer_action)
	push $(IPI_VEC_TIMER_ACTION)
	pushl $timer_action_handler
	jmp _interrupt_common
END_FUNC(timer_action)

# Generic IRQ vectors through the exception_handler function.
#
# Generate an array of ISR entry points for the assignable IRQ vectors, with
# each entry being 8 bytes long:
#
#   push <vec>            # 2 bytes
#   jmp <_irq_fn_common>  # 5 bytes
#   nop                   # 1 byte
#
# This is possible through the use of the "PUSH imm8" instruction, which encodes
# in 2 bytes. However, this instruction only operates on signed 8-bit values.
# Any value larger than INT8_MAX is interpreted by the assembler as a 16-bit
# immediate. Therefore, instead of using vectors 0-255, subtract 128 from the
# pushed values to keep them within the acceptable range.
#
# This table is referenced from C via the variable
#
#   extern uint64_t irq_fn[X86_NUM_ASSIGNABLE_VECTORS];
#
# in arch/i386/cpu/idt.c.

#define __VECTOR_CORRECTION 128

.align 8
BEGIN_FUNC(irq_fn)
vector = X86_FIRST_ASSIGNABLE_VECTOR
.rept X86_NUM_ASSIGNABLE_VECTORS
	push $(vector - __VECTOR_CORRECTION)
	jmp _irq_fn_common
	.align 8
	vector = vector + 1
.endr
END_FUNC(irq_fn)

# Fix the pushed vector value on the stack, then push the interrupt handler to
# put the stack into a valid state for _interrupt_common to process.
#
# Note: This *MUST* be defined directly before _interrupt_common to avoid a
#       second jump.
_irq_fn_common:
	addl $__VECTOR_CORRECTION, (%esp)
	push $interrupt_handler

# Common handling code for (most) interrupt vectors. At this point, two words
# are assumed to have already been pushed to the stack: the IRQ handler function
# and some custom vector-defined value (e.g. error code, vector number).
_interrupt_common:
	incl THIS_CPU_VAR(interrupt_depth)

	# Save the task's register state. After this, we're free to clobber.
	PUSH_STRUCT_REGS 8

	# Load the handler function into ecx and call it with a pointer to the
	# interrupt_context struct (eax) and the pushed error code (edx).
	# Following PUSH_STRUCT_REGS, eax stores the original esp (at the
	# location of the handler).
	movl (%eax), %ecx
	movl 4(%eax), %edx
	movl %esp, %eax
	pushl %edx
	pushl %eax
	cld
	call *%ecx
	addl $8, %esp

	# Restore the register state (of the original or new task).
	POP_STRUCT_REGS

	# Skip past the error code and handler function, returning to the
	# original interrupt stack.
	addl $8, %esp

	decl THIS_CPU_VAR(interrupt_depth)
	iret

# Special vectors that don't use the standard interrupt context (typically
# because they don't require it and have to be fast).

# IPI sent by a panicking processor to every other CPU in the system to shut
# them down.
BEGIN_FUNC(panic_shutdown)
	cli
	hlt
	jmp panic_shutdown
END_FUNC(panic_shutdown)

# TLB shootdown gate
BEGIN_FUNC(tlb_shootdown)
	cli
	push %eax
	push %edx

	# send EOI to APIC
	movl system_pic, %eax
	movl (%eax), %edx
	push $0
	call *%edx
	addl $4, %esp

	# TODO

	pop %edx
	pop %eax
	iret
END_FUNC(tlb_shootdown)
