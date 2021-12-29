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

#include <radix/asm/syscall.h>
#include <radix/assembler.h>

# Syscall interrupt handler for IA32 systems.
#
# Invoked from userspace with the syscall number in eax, first argument in ecx,
# and second argument in edx.
# TODO(frolv): Needs more args.
BEGIN_FUNC(syscall)
	incl THIS_CPU_VAR(interrupt_depth)

	push %ebp
	movl %esp, %ebp

	push %ds
	push %es

	# Align stack to 16 bytes and push arguments to the syscall function.
	subl $8, %esp
	push %edx
	push %ecx

	movl $0x10, %edx
	movw %dx, %ds
	movw %dx, %es

	cmpl $(X86_NUM_SYSCALLS), %eax
	jae .Linvalid_syscall

	movl syscall_table(, %eax, 4), %eax

	cld
	sti
	call *%eax

.Lreturn_to_user:
	xorl %ecx, %ecx
	xorl %edx, %edx
	addl $16, %esp

	pop %es
	pop %ds
	pop %ebp

	decl THIS_CPU_VAR(interrupt_depth)
	iret

.Linvalid_syscall:
	# TODO(frolv): Lots more to do here.
	movl $-1, %eax
	jmp .Lreturn_to_user
END_FUNC(syscall)
