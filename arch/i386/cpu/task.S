#
# arch/i386/cpu/task.S
# Copyright (C) 2016-2018 Alexei Frolov
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

.section .text
.align 4

.global switch_task
.type switch_task, @function

# switch_task(struct task *old, struct task *new)
# Switches the system from running task `old` to task `new`.
# Saves current registers from `old` loads and loads those from `new`.
# Starts executing at the new eip after returning.
# The struct regs within the struct task must begin at an offset of 32.
switch_task:
	push %ebp
	movl %esp, %ebp

	pushf

	push %eax
	movl 8(%ebp), %eax
	# Check if old is NULL. If it is, skip register saving.
	test %eax, %eax
	je load_registers

	# Store registers into old's struct regs.
	# Struct regs starts at offset of 32.
	movl %edi, 32(%eax)
	movl %esi, 36(%eax)
	# Value of esp before this function was called.
	# +8 for ebp and eip.
	movl %ebp, %edi
	addl $8, %edi
	movl %edi, 40(%eax)
	# Old value of ebp.
	movl 0(%ebp), %edi
	movl %edi, 44(%eax)
	movl %ebx, 48(%eax)
	movl %edx, 52(%eax)
	movl %ecx, 56(%eax)
	# eax that we saved on stack.
	movl 0(%esp), %edi
	movl %edi, 60(%eax)

	mov %gs, 64(%eax)
	mov %fs, 68(%eax)
	mov %es, 72(%eax)
	mov %ds, 76(%eax)
	mov %cs, 80(%eax)
	mov %ss, 84(%eax)

	# eip return address on stack.
	movl 4(%ebp), %edi
	movl %edi, 88(%eax)
	# EFLAGS
	movl 4(%esp), %edi
	movl %edi, 92(%eax)

# Load the new task struct.
load_registers:
	movl 12(%ebp), %eax

	# Switch over to the new thread's stack.
	# Make space for to store EFLAGS and EIP.
	movl 40(%eax), %ecx
	subl $8, %ecx
	movl %ecx, %esp

	# Store ecx and eax of new task on stack.
	movl 56(%eax), %ecx
	push %ecx
	movl 60(%eax), %ecx
	push %ecx

	# Load registers from new task struct.
	movl 36(%eax), %edi
	movl 36(%eax), %esi
	movl 44(%eax), %ebp
	movl 48(%eax), %ebx
	movl 52(%eax), %edx

	mov 64(%eax), %gs
	# skip fs
	mov 72(%eax), %es
	mov 76(%eax), %ds
	mov 84(%eax), %ss

	# Store task's EIP on the stack so that `ret` pops it and begins
	# executing where the task left off.
	movl 88(%eax), %ecx
	movl %ecx, 12(%esp)
	# Store task's EFLAGS register on the stack.
	movl 92(%eax), %ecx
	movl %ecx, 8(%esp)

	pop %eax
	pop %ecx

	popf
	cld
	ret
