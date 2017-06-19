#
# arch/i386/cpu/isrvec.S
# Copyright (C) 2016-2017 Alexei Frolov
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

.macro ISR_NOERR i
.global isr_\i
isr_\i:
	push $0
	push $\i
	jmp isr_common
.endm

.macro ISR_ERR i
.global isr_\i
isr_\i:
	push $\i
	jmp isr_common
.endm

# cpu exceptions
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30
ISR_NOERR 31

# ISA IRQs
.irp n,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47
	ISR_NOERR \n
.endr

# misc vectors
.irp n,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63
	ISR_NOERR \n
.endr
.irp n,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79
	ISR_NOERR \n
.endr
.irp n,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95
	ISR_NOERR \n
.endr
.irp n,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111
	ISR_NOERR \n
.endr
.irp n,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127
	ISR_NOERR \n
.endr

# system call interrupt
ISR_NOERR 128

# misc vectors cont.
.irp n,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143
	ISR_NOERR \n
.endr
.irp n,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159
	ISR_NOERR \n
.endr
.irp n,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175
	ISR_NOERR \n
.endr
.irp n,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191
	ISR_NOERR \n
.endr
.irp n,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207
	ISR_NOERR \n
.endr
.irp n,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223
	ISR_NOERR \n
.endr
.irp n,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239
	ISR_NOERR \n
.endr
.irp n,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
	ISR_NOERR \n
.endr


.align 16
isr_common:
	testw $0x3, 12(%esp)
	jz stack_fix

# Push all registers onto the stack as part of a struct regs.
# The order in which these are pushed is important; see
# arch/i386/include/radix/asm/regs.h for the struct definition.
register_push:
	# segment registers
	push %ds
	push %es
	push %fs
	push %gs

	# general purpose registers
	pushl %eax
	pushl %ecx
	pushl %edx
	pushl %ebx
	pushl %ebp
	pushl %esi
	pushl %edi

	cld
	call interrupt_handler

register_pop:
	popl %edi
	popl %esi
	popl %ebp
	popl %ebx
	popl %edx
	popl %ecx
	popl %eax

	pop %gs
	# skip over fs; it's used for per-cpu storage
	addl $4, %esp
	pop %es
	pop %ds

	# for error code and interrupt number
	addl $8, %esp

	testw $0x3, 4(%esp)
	jz stack_restore

iret_final:
	iret

# Interrupts from ring level 0 to level 0 do not push ss or esp.
# Move the stack down 8 bytes and manually push them.
# Adapted from Sortix kernel/x86/interrupt.S.
stack_fix:
	# Save the current value of eax.
	movl %eax, -12(%esp)
	# intno
	movl 0(%esp), %eax
	movl %eax, -8(%esp)
	# errno
	movl 4(%esp), %eax
	movl %eax, -4(%esp)
	# eip
	movl 8(%esp), %eax
	movl %eax, 0(%esp)
	# cs
	movl 12(%esp), %eax
	movl %eax, 4(%esp)
	# eflags
	movl 16(%esp), %eax
	movl %eax, 8(%esp)

	# Push the values of esp and ss.
	movl %esp, %eax
	addl $20, %eax
	movl %eax, 12(%esp)
	movl %ss, %eax
	movl %eax, 16(%esp)

	# Reload eax, fix stack pointer and handle the interrupt.
	movl -12(%esp), %eax
	subl $8, %esp
	jmp register_push

# When returning from an interrupt, the pushed ss and esp need to be
# loaded and the stack restored to how iret expects it to be.
# This is accomplished by copying the eip, cs and eflags that the interrupt
# pushed to the new location of the stack pointer and loading it.
# Once again, taken from Sortix kernel/x86/interrupt.S.
stack_restore:
	# Save eax, ebx and ecx values.
	movl %eax, -4(%esp)
	movl %ebx, -8(%esp)
	movl %ecx, -12(%esp)

	# New stack pointer.
	movl 12(%esp), %ebx
	subl $12, %ebx
	movw 16(%esp), %cx

	# Copy saved eax, ebx and ecx onto the new stack.
	movl -4(%esp), %eax
	movl %eax, -12(%ebx)
	movl -8(%esp), %eax
	movl %eax, -16(%ebx)
	movl -12(%esp), %eax
	movl %eax, -20(%ebx)

	# Copy interrupted eflags, cs and eip onto the new stack.
	movl 8(%esp), %eax
	movl %eax, 8(%ebx)
	movl 4(%esp), %eax
	movl %eax, 4(%ebx)
	movl 0(%esp), %eax
	movl %eax, 0(%ebx)

	# Load the two values we pushed earlier.
	mov %cx, %ss
	movl %ebx, %esp

	# Restore e{a,b,c}x.
	movl -12(%esp), %eax
	movl -16(%esp), %ebx
	movl -20(%esp), %ecx

	# The new stack is now set up, ready for an iret.
	jmp iret_final
