#
# arch/i386/irq/intgates.S
# Copyright (C) 2017 Alexei Frolov
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

#include <radix/assembler.h>
#include <radix/irq.h>

.macro UNHANDLED_EXCEPTION
	call exception_unhandled
	iret
.endm

# per-CPU variable to track number of unhandled exceptions
# TODO: when APs come online, they should reset this to zero
DEFINE_PER_CPU(unhandled_exceptions)
	.long 0
DEFINE_PER_CPU_END()

DEFINE_PER_CPU(interrupt_depth)
	.long 0
DEFINE_PER_CPU_END()

# function for unimplemented exception handlers
exception_unhandled:
	incl THIS_CPU_VAR(unhandled_exceptions)
	ret

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

#define REGS_DI(base)           (base)
#define REGS_SI(base)          4(base)
#define REGS_SP(base)          8(base)
#define REGS_BP(base)         12(base)
#define REGS_BX(base)         16(base)
#define REGS_DX(base)         20(base)
#define REGS_CX(base)         24(base)
#define REGS_AX(base)         28(base)
#define REGS_GS(base)         32(base)
#define REGS_FS(base)         36(base)
#define REGS_ES(base)         40(base)
#define REGS_DS(base)         44(base)
#define REGS_CS(base)         48(base)
#define REGS_SS(base)         52(base)
#define REGS_IP(base)         56(base)
#define REGS_FLAGS(base)      60(base)

# Build a struct regs from an interrupt context.
# `pushed_bytes` is the number of bytes already pushed
# onto the stack since the interrupt.
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
	# skip fs and gs
	addl $8, %esp
	pop %es
	pop %ds
	# skip cs
	addl $4, %esp
	pop %ss
	# skip eip and eflags
	addl $8, %esp
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

# Generate an array of interrupt entry point functions,
# with each entry 8 bytes long.
.align 8
BEGIN_FUNC(irq_fn)
vector = IRQ_BASE
.rept (NUM_INTERRUPT_VECTORS - NUM_EXCEPTION_VECTORS)
	push $(vector - 0x80)
	jmp interrupt_common
	.align 8
	vector = vector + 1
.endr
END_FUNC(irq_fn)

interrupt_common:
	incl THIS_CPU_VAR(interrupt_depth)
	addl $0x80, (%esp)
	BUILD_STRUCT_REGS 4
	movl 64(%esp), %eax
	movl %esp, %edx
	pushl %eax
	pushl %edx
	cld
	call interrupt_handler
	addl $8, %esp
	POP_STRUCT_REGS
	addl $4, %esp
	decl THIS_CPU_VAR(interrupt_depth)
	iret


# special interrupt vectors

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

#
# The interrupt used by radix's event system.
# Unlike the interrupts above, this may result in a context switch and should
# allow the values pushed to the stack by the interrupt to be modified from C.
#
BEGIN_FUNC(event_irq)
	incl THIS_CPU_VAR(interrupt_depth)

	# check if the interrupt occurred from user mode
	testw $0x3, 4(%esp)
	jnz .Lpush_regs

	# When an interrupt occurs in ring 0, ss and esp are not pushed.
	# Move the stack down by 8 bytes and manually insert them.
	movl %eax, -12(%esp)
	movl (%esp), %eax
	movl %eax, -8(%esp)
	movl 4(%esp), %eax
	movl %eax, -4(%esp)
	movl 8(%esp), %eax
	movl %eax, (%esp)

	# Insert ss and esp.
	movl %esp, %eax
	addl $12, %eax
	movl %eax, 4(%esp)
	movl %ss, %eax
	movl %eax, 8(%esp)
	movl -12(%esp), %eax
	subl $8, %esp

.Lpush_regs:
	BUILD_STRUCT_REGS 0
	pushl %esp
	cld
	call arch_event_handler
	addl $4, %esp
	POP_STRUCT_REGS

	testw $0x3, 4(%esp)
	jnz .Lfinish_irq

	# When returning from a ring 0 interrupt, the new ss and esp need to be
	# loaded and the stack restored to a valid iret state. This is done by
	# copying the stored eip, cs and eflags onto the new stack and then
	# switching over to it.
	movl %eax, -4(%esp)
	movl %ebx, -8(%esp)
	movl %edx, -12(%esp)

	# Load the new stack and copy values over.
	movl 12(%esp), %ebx
	subl $12, %ebx
	movw 16(%esp), %dx
	movl -4(%esp), %eax
	movl %eax, -12(%ebx)
	movl -8(%esp), %eax
	movl %eax, -16(%ebx)
	movl -12(%esp), %eax
	movl %eax, -20(%ebx)
	movl 8(%esp), %eax
	movl %eax, 8(%ebx)
	movl 4(%esp), %eax
	movl %eax, 4(%ebx)
	movl (%esp), %eax
	movl %eax, (%ebx)

	# Switch to the new stack.
	mov %dx, %ss
	movl %ebx, %esp
	movl -12(%esp), %eax
	movl -16(%esp), %ebx
	movl -20(%esp), %edx

.Lfinish_irq:
	decl THIS_CPU_VAR(interrupt_depth)
	iret
END_FUNC(event_irq)
