#
# arch/i386/boot.S
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

# Constants for multiboot header.
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
.section .bootstrap_stack, "aw", @nobits
.align 16
stack_bottom:
.skip 0x4000
stack_top:

.set KERNEL_VIRTUAL_BASE, 0xC0000000

.section .bss, "aw", @nobits
.align 0x1000
# x86 top-level page directory.
pgdir:
	.fill 0x400, 4, 0x0
# A single page table to map the kernel's memory.
kernel_pgtbl:
	.fill 0x400, 4, 0x0

.section .text
.global _start
.type _start, @function
_start:
	# Physical address of the kernel's page table.
	movl $(kernel_pgtbl - KERNEL_VIRTUAL_BASE), %edi
	xorl %esi, %esi
	movl $0x400, %ecx

1:
	# Mark the address as global, present and writeable.
	movl %esi, %edx
	orl $0x00000103, %edx

	# Clear writeable bit for read-only sections.
	cmpl $(__text_start - KERNEL_VIRTUAL_BASE), %esi
	jl do_move
	cmpl $(__rodata_end - KERNEL_VIRTUAL_BASE), %esi
	jge do_move
	andl $0xFFFFFFFD, %edx

do_move:
	movl %edx, (%edi)

	# Advance to the next page.
	addl $0x1000, %esi
	addl $4, %edi
	loop 1b

2:
	# Map the page table to page directory entry 0
	# (virtual addresses 0x00000000 to 0x003FFFFF),
	# identity mapping the kernel. This is done because
	# enabling paging does not change the address of the
	# next instruction, which continues to be physical.
	movl $(kernel_pgtbl - KERNEL_VIRTUAL_BASE), %ecx
	orl $0x00000103, %ecx
	movl %ecx, pgdir - KERNEL_VIRTUAL_BASE

	# Map the page table to page directory entry 0x300
	# (virtual addresses 0xC0000000 to 0xC03FFFFF),
	# mapping the kernel in the higher half.
	movl $(kernel_pgtbl - KERNEL_VIRTUAL_BASE), %ecx
	orl $0x00000103, %ecx
	movl %ecx, pgdir - KERNEL_VIRTUAL_BASE + 0x300 * 4

	# Map the final entry in the page directory to
	# the page directory itself.
	movl $(pgdir - KERNEL_VIRTUAL_BASE), %ecx
	orl $0x00000103, %ecx
	movl %ecx, pgdir - KERNEL_VIRTUAL_BASE + 0x3FF * 4

	# Load the physical address of the page directory into cr3.
	movl $(pgdir - KERNEL_VIRTUAL_BASE), %ecx
	movl %ecx, %cr3

	# Enable paging and the write protect bit,
	# preventing writing to read-only pages.
	movl %cr0, %ecx
	orl $0x80010000, %ecx
	movl %ecx, %cr0

	# Jump to the higher half addresses.
	leal higher_half_start, %ecx
	jmp *%ecx

higher_half_start:
	# Remove the identity mapped-page and flush the TLB.
	movl $0, pgdir
	invlpg (0)

	# Set stack pointer to point to the top of the stack.
	movl $stack_top, %esp
	cld

	# Call global constructors.
	call _init

	# Initialize basic structures for the bootstrap processor.
	call bsp_init_early
	sti

	call vgatext_register

kernel_start:
	# ebx contains the physical address of the multiboot_info
	# structure. Covert this to its equivalent virtual address.
	addl $KERNEL_VIRTUAL_BASE, %ebx
	pushl %ebx
	# Launch the kernel.
	call kmain
	addl $8, %esp

	# Hang if kmain returns.
	cli
1:	hlt
	jmp 1b

# set the size of _start symbol to the current location '.' minus its start
.size _start, . - _start
