#
# arch/i386/boot.S
# Copyright (C) 2016 Alexei Frolov
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

# constants for multiboot header
.set ALIGN,	1 << 0
.set MEMINFO,	1 << 1
.set FLAGS,	ALIGN | MEMINFO
.set MAGIC,	0x1BADB002
.set CHECKSUM,	-(MAGIC + FLAGS)

# Declare a multiboot header that marks the program as a kernel.
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Allocate room for a small stack by creating a symbol at the bottom,
# skipping ahead 16 KiB and creating a symbol at the top. The stack
# must be 16-byte aligned on x86.
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

.section .text
.global _start
.type _start, @function
_start:
	# set stack pointer to point to the top of the stack
	movl $stack_top, %esp

	# call global constructors
	call _init

	# intitialize the tty
	call tty_init

# load the global descriptor table
load_gdt:
	call gdt_init

	# print a message indicating successful gdt load
	push len_gdt_msg
	push $gdt_msg
	call tty_write
	addl $8, %esp

# load the interrupt descriptor table
load_idt:
	call idt_init

	# print a message indicating successful idt load
	push len_idt_msg
	push $idt_msg
	call tty_write
	addl $8, %esp

kernel_start:
	# Launch the kernel.
	call kmain

	# Hang if kmain returns.
	cli
1:	hlt
	jmp 1b

# set the size of _start symbol to the current location '.' minus its start
.size _start, . - _start

.section .data
gdt_msg:
	.ascii "[GDT] success\n"
len_gdt_msg:
	.long . - gdt_msg
idt_msg:
	.ascii "[IDT] success\n"
len_idt_msg:
	.long . - idt_msg
