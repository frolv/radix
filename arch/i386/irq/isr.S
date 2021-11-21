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

.macro POP_STRUCT_REGS
	popl %edi
	popl %esi
	# skip original esp
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
	# Skip cs, ss, eip, and eflags, as they'll be handled by the iret.
	addl $16, %esp
.endm

# Function for unimplemented exception handlers.
exception_unhandled:
	incl THIS_CPU_VAR(unhandled_exceptions)
	ret

# TODO(frolv): Begin legacy code. Should be migrated to use _interrupt_common.

BEGIN_FUNC(div_error)
	pushl $0
	pushl $div_error_handler
	jmp exception_common
END_FUNC(div_error)

BEGIN_FUNC(debug)
	UNHANDLED_EXCEPTION
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
	UNHANDLED_EXCEPTION
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
	UNHANDLED_EXCEPTION
END_FUNC(general_protection_fault)

BEGIN_FUNC(page_fault)
	pushl $page_fault_handler
	jmp exception_common
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

# Build a struct regs from an interrupt context.
# `pushed_bytes` is the number of bytes already pushed
# onto the stack since the interrupt.
#
# TODO(frolv): This is wrong and should be removed in favor of PUSH_STRUCT_REGS.
.macro BUILD_STRUCT_REGS pushed_bytes
	# space for eflags and eip
	subl $8, %esp
	push %ss
	# space for cs
	subl $4, %esp
	push %ds
	push %es
	push %fs
	push %gs
	pushl %eax
	pushl %ecx
	pushl %edx
	pushl %ebx
	pushl %ebp
	# original esp
	leal (52 + \pushed_bytes)(%esp), %eax
	pushl %eax
	pushl %esi
	pushl %edi
	# saved eflags
	movl (72 + \pushed_bytes)(%esp), %eax
	movl %eax, REGS_FLAGS(%esp)
	# saved cs
	movl (68 + \pushed_bytes)(%esp), %eax
	movl %eax, REGS_CS(%esp)
	# saved eip
	movl (64 + \pushed_bytes)(%esp), %eax
	movl %eax, REGS_IP(%esp)
.endm

# store GPRs, call exception handler function, and restore context
exception_common:
	incl THIS_CPU_VAR(interrupt_depth)
	BUILD_STRUCT_REGS 8
	# exception error code in eax, handler function in ecx,
	# base of struct regs in edx
	movl 68(%esp), %eax
	movl 64(%esp), %ecx
	movl %esp, %edx
	pushl %eax
	pushl %edx
	cld
	call *%ecx
	addl $8, %esp
	POP_STRUCT_REGS
	addl $8, %esp
	decl THIS_CPU_VAR(interrupt_depth)
	iret

BEGIN_FUNC(lapic_error)
	pushl $lapic_error_handler
	jmp irq_noargs_common
END_FUNC(lapic_error)

irq_noargs_common:
	incl THIS_CPU_VAR(interrupt_depth)
	BUILD_STRUCT_REGS 4
	movl 64(%esp), %eax
	cld
	call *%eax
	POP_STRUCT_REGS
	addl $4, %esp
	decl THIS_CPU_VAR(interrupt_depth)
	iret

# TODO(frolv): End of legacy code.

# Creates a `struct regs` on the stack with the values of the current registers
# and the context pushed at the start of an interrupt.
#
# This assumes a 5-byte interrupt context: ss, sp, flags, cs, ip. In x86, this
# only occurs automatically when an interrupt occurs at a lower privilige level.
# To use this for interrupts at CPL 0, a "stack fixup" must first be performed.
.macro PUSH_STRUCT_REGS pushed_bytes
	# Reserve space for eflags, eip, ss, and cs.
	subl $16, %esp
	push %ds
	push %es
	push %fs
	push %gs
	pushl %eax
	pushl %ecx
	pushl %edx
	pushl %ebx
	pushl %ebp
	# Reserve space for esp.
	subl $4, %esp
	pushl %esi
	pushl %edi

	# Copy the five values from the interrupt context into the struct.
	movl (80 + \pushed_bytes)(%esp), %eax
	movl %eax, REGS_SS(%esp)
	movl (76 + \pushed_bytes)(%esp), %eax
	movl %eax, REGS_SP(%esp)
	movl (72 + \pushed_bytes)(%esp), %eax
	movl %eax, REGS_FLAGS(%esp)
	movl (68 + \pushed_bytes)(%esp), %eax
	movl %eax, REGS_CS(%esp)
	movl (64 + \pushed_bytes)(%esp), %eax
	movl %eax, REGS_IP(%esp)
.endm

#
# Custom kernel IRQ vectors.
#

BEGIN_FUNC(event_irq)
	push $APIC_VEC_TIMER
	push $arch_event_handler
	jmp _interrupt_common
END_FUNC(event_irq)

BEGIN_FUNC(sched_wake)
	push $IPI_VEC_SCHED_WAKE
	push $sched_wake_handler
	jmp _interrupt_common
END_FUNC(sched_wake)

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
# and some custom vector-defined value (e.g. error code, vector number,
_interrupt_common:
	incl THIS_CPU_VAR(interrupt_depth)

	# Check if the interrupt occurred from user mode (ring 3). If it did,
	# the stack is properly configured, so skip ahead.
	testw $0x3, 12(%esp)
	jnz .Lcall_handler

	# Interrupts occuring in ring 0 do not push ss and esp to the stack like
	# their ring 3 counterparts.
	#
	# To support uniform handling for both kernel and user threads, ring 0
	# interrupts imitate the ring 3 stack by inserting the additional
	# registers where they would otherwise have appeared.
	#
	# Ring 0 interrupt transformation:
	#
	#   1. Initial stack on interrupt entry.
	#
	#     +--------+--------+--------+--------+--------+
	#     | eflags |   cs   |   ip   |  code  |  func  |
	#     +--------+--------+--------+--------+--------+
	#             16       12        8        4        0
	#                                                  ^
	#
	#   2. Shift everything downwards by 8 bytes to make space for the two
	#      registers. This requires stashing eax at the bottom of the stack
	#      as it will be clobbered.
	#
	#      +--------+--------+--------+--------+--------+--------+--------+
	#      |        |        | eflags |   cs   |   ip   |  code  |  func  |
	#      +--------+--------+--------+--------+--------+--------+--------+
	#              16       12        8        4        0       -4       -8
	#                                                   ^
	#
	#   3. Insert ss and esp into the new spaces. 20 bytes are added to the
	#      stored esp's value so that it points to where it was before the
	#      interrupt was triggered.
	#
	#      +--------+--------+--------+--------+--------+--------+--------+
	#      |   ss   |   sp   | eflags |   cs   |   ip   |  code  |  func  |
	#      +--------+--------+--------+--------+--------+--------+--------+
	#              16       12        8        4        0       -4       -8
	#                                                   ^
	#
	#   4. Advance the active esp into to its new position and restore the
	#      stashed eax. The stack now mimics that of a ring 3 interrupt.
	#
	#      +--------+--------+--------+--------+--------+--------+--------+
	#      |   ss   |   sp   | eflags |   cs   |   ip   |  code  |  func  |
	#      +--------+--------+--------+--------+--------+--------+--------+
	#              24       20       16       12        8        4        0
	#                                                                     ^
	#
	#   5. From here, continue into the main interrupt handler, which will
	#      save and potentially modify these stored values.
	#

	# Stash eax and shift everything down.
	movl %eax, -12(%esp)
	movl (%esp), %eax    # func
	movl %eax, -8(%esp)
	movl 4(%esp), %eax   # code
	movl %eax, -4(%esp)
	movl 8(%esp), %eax   # ip
	movl %eax, (%esp)
	movl 12(%esp), %eax  # cs
	movl %eax, 4(%esp)
	movl 16(%esp), %eax  # eflags
	movl %eax, 8(%esp)

	# Insert the original value of esp and ss.
	leal 20(%esp), %eax
	movl %eax, 12(%esp)
	movl %ss, %eax
	movl %eax, 16(%esp)

	# Restore eax and move esp to its final position.
	movl -12(%esp), %eax
	subl $8, %esp

.Lcall_handler:
	# Save the task's register state. After this, we're free to clobber.
	PUSH_STRUCT_REGS 8

	# Load the handler function into ecx and call it with a pointer to the
        # regs struct (eax) and the pushed error code (edx). Load pushed values
	# relative to the original esp from the regs struct to be resilient to
	# any changes in its layout. (Refer to the "diagrams" above.)
	movl %esp, %eax
	movl REGS_SP(%eax), %edx
	movl -28(%edx), %ecx
	movl -24(%edx), %edx
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

	# Check the cs on the stack to see if returning to a ring 3 or ring 0
	# context. For ring 3, there is nothing to do, as the stack is already
	# correctly set up.
	testw $0x3, 4(%esp)
	jnz .Lfinish_interrupt

	# When returning into a ring 0 context, we may be switching onto a
	# different stack and must handle this manually. Copy the three pushed
	# interrupt context values (flags, cs, ip) below the new stack pointer
	# (which may be the current stack) and switch to it.
	#
	# TODO(frolv): This operation requires 5 words of temporary space on the
	# new stack and 2 on the current stack. This should be checked first.

	# Stash ebx and eax.
	push %ebx
	push %eax

	# Load the new thread's stack pointer into ebx.
	movl 20(%esp), %ebx

	# Copy over the context from the current stack onto the new stack.
	# This MUST be done from top-to-bottom, as the two stacks may be the
	# same, in which case the values are simply being shifted up by 2 bytes.
	movl 16(%esp), %eax  # eflags
	movl %eax, -4(%ebx)
	movl 12(%esp), %eax  # cs
	movl %eax, -8(%ebx)
	movl 8(%esp), %eax   # eip
	movl %eax, -12(%ebx)

	# Load the kernel's ss.
	mov $0x10, %ax
	mov %ax, %ss

	# Finally, switch over to the new stack and restore eax and ebx.
	movl %esp, %eax
	subl $12, %ebx
	movl %ebx, %esp
	movl 4(%eax), %ebx
	movl (%eax), %eax

.Lfinish_interrupt:
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

BEGIN_FUNC(timer_action)
	cli
	pushl $timer_action_handler
	jmp irq_noargs_common
END_FUNC(timer_action)
