#
# arch/i386/cpu/task.S
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

#include <radix/asm/regs_asm.h>

.section .text
.align 4

.global switch_task
.type switch_task, @function

# The offset of a regs struct within a task struct.
# If radix/task.h is ever modified, this should be updated.
#define REGS_OFFSET 32

# void switch_task(struct task *old, struct task *new);
#
# Switches the system from running task `old` to task `new`.
# Saves current registers from `old` and loads those from `new`.
# Starts executing at the new eip after returning.
switch_task:
	push %ebp
	movl %esp, %ebp

	pushf
	cli

	# Load old into eax, storing its original value for later use.
	push %eax
	movl 8(%ebp), %eax

	# Check if old is NULL. If it is, skip register saving.
	test %eax, %eax
	je .Lload_registers

	# Store registers into old's struct regs.
	movl %edi, REGS_DI(%eax, REGS_OFFSET)
	movl %esi, REGS_SI(%eax, REGS_OFFSET)

	# Value of esp before this function was called.
	# +8 for ebp and eip.
	movl %ebp, %edi
	addl $8, %edi
	movl %edi, REGS_SP(%eax, REGS_OFFSET)

	# Old value of ebp.
	movl (%ebp), %edi
	movl %edi, REGS_BP(%eax, REGS_OFFSET)

	movl %ebx, REGS_BX(%eax, REGS_OFFSET)
	movl %edx, REGS_DX(%eax, REGS_OFFSET)
	movl %ecx, REGS_CX(%eax, REGS_OFFSET)

	# eax that we saved on the stack.
	movl (%esp), %edi
	movl %edi, REGS_AX(%eax, REGS_OFFSET)

	mov %gs, REGS_GS(%eax, REGS_OFFSET)
	mov %fs, REGS_FS(%eax, REGS_OFFSET)
	mov %es, REGS_ES(%eax, REGS_OFFSET)
	mov %ds, REGS_DS(%eax, REGS_OFFSET)
	mov %cs, REGS_CS(%eax, REGS_OFFSET)
	mov %ss, REGS_SS(%eax, REGS_OFFSET)

	# eip return address on stack.
	movl 4(%ebp), %edi
	movl %edi, REGS_IP(%eax, REGS_OFFSET)
	# Pushed eflags value.
	movl -4(%ebp), %edi
	movl %edi, REGS_FLAGS(%eax, REGS_OFFSET)

.Lload_registers:
	# Load the new task.
	movl 12(%ebp), %eax

	# Switch over to the new thread's stack.
	# Make space for to store EFLAGS and EIP.
	movl REGS_SP(%eax, REGS_OFFSET), %ecx
	subl $8, %ecx
	movl %ecx, %esp

	# Store ecx and eax of new task on stack.
	movl REGS_CX(%eax, REGS_OFFSET), %ecx
	push %ecx
	movl REGS_AX(%eax, REGS_OFFSET), %ecx
	push %ecx

	# Load registers from new task struct.
	movl REGS_DI(%eax, REGS_OFFSET), %edi
	movl REGS_SI(%eax, REGS_OFFSET), %esi
	movl REGS_BP(%eax, REGS_OFFSET), %ebp
	movl REGS_BX(%eax, REGS_OFFSET), %ebx
	movl REGS_DX(%eax, REGS_OFFSET), %edx

	# Load the segment registers, skipping fs as it's set per CPU.
	mov REGS_GS(%eax, REGS_OFFSET), %gs
	mov REGS_ES(%eax, REGS_OFFSET), %es
	mov REGS_DS(%eax, REGS_OFFSET), %ds
	mov REGS_SS(%eax, REGS_OFFSET), %ss

	# Store task's EIP on the stack so that `ret` pops it and begins
	# executing where the task left off.
	movl REGS_IP(%eax, REGS_OFFSET), %ecx
	movl %ecx, 12(%esp)
	# Store task's EFLAGS register on the stack.
	movl REGS_FLAGS(%eax, REGS_OFFSET), %ecx
	movl %ecx, 8(%esp)

	pop %eax
	pop %ecx

	popf
	cld
	ret
