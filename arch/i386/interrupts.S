#
# arch/i386/cpu/interrupts.S
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

.section .text
.align 4

# no-op function for unimplemented exception handlers
exception_nop:
	ret

BEGIN_FUNC(div_error)
	push $0
	push $exception_nop
	jmp exception_common
END_FUNC(div_error)

BEGIN_FUNC(debug)
	push $0
	push $exception_nop
	jmp exception_common
END_FUNC(debug)

BEGIN_FUNC(breakpoint)
	push $0
	push $exception_nop
	jmp exception_common
END_FUNC(breakpoint)

BEGIN_FUNC(overflow)
	push $0
	push $exception_nop
	jmp exception_common
END_FUNC(overflow)

BEGIN_FUNC(bound_range)
	push $0
	push $exception_nop
	jmp exception_common
END_FUNC(bound_range)

BEGIN_FUNC(invalid_opcode)
	push $0
	push $exception_nop
	jmp exception_common
END_FUNC(invalid_opcode)

BEGIN_FUNC(device_not_available)
	push $0
	push $exception_nop
	jmp exception_common
END_FUNC(device_not_available)

BEGIN_FUNC(double_fault)
	push $exception_nop
	jmp exception_common
END_FUNC(double_fault)

BEGIN_FUNC(coprocessor_segment)
	push $0
	push $exception_nop
	jmp exception_common
END_FUNC(coprocessor_segment)

BEGIN_FUNC(invalid_tss)
	push $exception_nop
	jmp exception_common
END_FUNC(invalid_tss)

BEGIN_FUNC(segment_not_present)
	push $exception_nop
	jmp exception_common
END_FUNC(segment_not_present)

BEGIN_FUNC(stack_segment)
	push $exception_nop
	jmp exception_common
END_FUNC(stack_segment)

BEGIN_FUNC(general_protection_fault)
	push $exception_nop
	jmp exception_common
END_FUNC(general_protection_fault)

BEGIN_FUNC(page_fault)
	push $page_fault_handler
	jmp exception_common
END_FUNC(page_fault)

BEGIN_FUNC(x87_floating_point)
	push $0
	push $exception_nop
	jmp exception_common
END_FUNC(x87_floating_point)

BEGIN_FUNC(alignment_check)
	push $exception_nop
	jmp exception_common
END_FUNC(alignment_check)

BEGIN_FUNC(machine_check)
	push $0
	push $exception_nop
	jmp exception_common
END_FUNC(machine_check)

BEGIN_FUNC(simd_floating_point)
	push $0
	push $exception_nop
	jmp exception_common
END_FUNC(simd_floating_point)

BEGIN_FUNC(virtualization_exception)
	push $0
	push $exception_nop
	jmp exception_common
END_FUNC(virtualization_exception)

BEGIN_FUNC(security_exception)
	push $exception_nop
	jmp exception_common
END_FUNC(security_exception)

# store GPRs, call exception handler function, and restore context
exception_common:
	# create a struct regs starting here
	subl $8, %esp
	push %ss
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

	# push the original value of esp
	movl %esp, %eax
	addl $60, %eax
	pushl %eax

	pushl %esi
	pushl %edi

	# saved eflags
	movl 80(%esp), %eax
	movl %eax, 60(%esp)
	# saved cs
	movl 76(%esp), %eax
	movl %eax, 48(%esp)
	# saved eip
	movl 72(%esp), %eax
	movl %eax, 56(%esp)

	# exception error code in eax, handler function in ecx,
	# base of struct regs in eax
	movl 68(%esp), %eax
	movl 64(%esp), %ecx
	movl %esp, %edx
	pushl %eax
	pushl %edx
	cld
	call *%ecx
	addl $8, %esp

	popl %edi
	popl %esi
	addl $4, %esp
	popl %ebp
	popl %ebx
	popl %edx
	popl %ecx
	popl %eax
	pop %gs
	addl $4, %esp
	pop %es
	pop %ds
	addl $4, %esp
	pop %ss
	addl $16, %esp

	iret
