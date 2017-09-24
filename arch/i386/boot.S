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

#include <radix/assembler.h>
#include <radix/asm/page_defs.h>

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
.align PAGE_SIZE
bsp_stack_bottom:
.skip PAGE_SIZE * 4
.global bsp_stack_top
bsp_stack_top:

.set KERNEL_VIRTUAL_BASE, 0xC0000000
#define PHYS(x) (x - KERNEL_VIRTUAL_BASE)

.section .bss, "aw", @nobits
#ifdef CONFIG_PAE

# Top-level page directory pointer table.
# Only takes up 32 bytes, but a whole physical page is
# reserved for it to make recursive mapping simpler.
.align PAGE_SIZE
.global kernel_pdpt
kernel_pdpt:
	.fill 0x200, 8, 0x0

.align PAGE_SIZE
pgdir_0:
	.fill PTRS_PER_PGDIR, 8, 0x0
pgdir_1:
	.fill PTRS_PER_PGDIR, 8, 0x0
pgdir_2:
	.fill PTRS_PER_PGDIR, 8, 0x0
kernel_pgdir:
	.fill PTRS_PER_PGDIR, 8, 0x0

# Two page tables are required to map the 4 MiB of kernel memory.
kernel_pgtbl:
	.fill PTRS_PER_PGTBL, 8, 0x0
	.fill PTRS_PER_PGTBL, 8, 0x0

#else

.align PAGE_SIZE
# x86 top-level page directory.
.global kernel_pgdir
kernel_pgdir:
	.fill PTRS_PER_PGDIR, 4, 0x0
# A single page table to map the kernel's memory.
kernel_pgtbl:
	.fill PTRS_PER_PGTBL, 4, 0x0

#endif /* CONFIG_PAE */

.section .text
#ifdef CONFIG_PAE

paging_init:
	movl %eax, %edi
	movl %ebx, %esi
	movl $1, %eax
	cpuid
	testb $0x40, %dl
	jz .Lno_pae_support
	movl %esi, %ebx
	movl %edi, %eax

	# Populate the PDPT with four present page directory entries.
	movl $(PHYS(kernel_pdpt)), %edi
	movl $(PHYS(pgdir_0)), %edx
	orl $0x00000001, %edx
	movl %edx, (%edi)
	movl $(PHYS(pgdir_1)), %edx
	orl $0x00000001, %edx
	movl %edx, 8(%edi)
	movl $(PHYS(pgdir_2)), %edx
	orl $0x00000001, %edx
	movl %edx, 16(%edi)
	movl $(PHYS(kernel_pgdir)), %edx
	orl $0x00000001, %edx
	movl %edx, 24(%edi)

	# Populate the two kernel page tables.
	movl $(PHYS(kernel_pgtbl)), %edi
	xorl %esi, %esi
	movl $(PTRS_PER_PGTBL * 2), %ecx

1:
	movl %esi, %edx
	orl $0x00000103, %edx

	cmpl $(PHYS(__text_start)), %esi
	jl 2f
	cmpl $(PHYS(__rodata_end)), %esi
	jge 2f
	andl $0xFFFFFFFD, %edx

2:
	movl %edx, (%edi)
	# TODO: set NX bit
	movl $0, 4(%edi)

	# Advance to the next page.
	addl $(PAGE_SIZE), %esi
	addl $8, %edi
	loop 1b

	# Temporarily identity map the first kernel page table.
	movl $(PHYS(kernel_pgtbl)), %ecx
	orl $0x00000103, %ecx
	movl %ecx, PHYS(pgdir_0)
	movl %ecx, PHYS(kernel_pgdir)

	# Map the second kernel page table.
	movl $(PHYS(kernel_pgtbl) + PAGE_SIZE), %ecx
	orl $0x00000103, %ecx
	movl %ecx, PHYS(kernel_pgdir) + 8

	# Recursive map the paging structures.
	movl $(PHYS(kernel_pgdir)), %ecx
	orl $0x00000103, %ecx
	movl %ecx, PHYS(kernel_pgdir) + 511 * 8
	movl $(PHYS(pgdir_2)), %ecx
	orl $0x00000103, %ecx
	movl %ecx, PHYS(kernel_pgdir) + 510 * 8
	movl $(PHYS(pgdir_1)), %ecx
	orl $0x00000103, %ecx
	movl %ecx, PHYS(kernel_pgdir) + 509 * 8
	movl $(PHYS(pgdir_0)), %ecx
	orl $0x00000103, %ecx
	movl %ecx, PHYS(kernel_pgdir) + 508 * 8

	movl $(PHYS(kernel_pdpt)), %ecx
	movl %ecx, %cr3
	orl $0x00000103, %ecx
	movl %ecx, PHYS(kernel_pgdir) + 507 * 8

	# Enable PAE.
	movl %cr4, %ecx
	orl $0x00000020, %ecx
	movl %ecx, %cr4

	# Enable paging and the write protect bit,
	# preventing writing to read-only pages.
	movl %cr0, %ecx
	orl $0x80010000, %ecx
	movl %ecx, %cr0

	# Jump to the higher half addresses.
	leal higher_half_start, %ecx
	jmp *%ecx

.Lno_pae_support:
#else
paging_init:
#endif /* CONFIG_PAE */

	# Populate the kernel page table.
	movl $(PHYS(kernel_pgtbl)), %edi
	xorl %esi, %esi
	movl $(PTRS_PER_PGTBL), %ecx

1:
	# Mark the address as global, present and writeable.
	movl %esi, %edx
	orl $0x00000103, %edx

	# Clear writeable bit for read-only sections.
	cmpl $(PHYS(__text_start)), %esi
	jl .Ldo_move
	cmpl $(PHYS(__rodata_end)), %esi
	jge .Ldo_move
	andl $0xFFFFFFFD, %edx

.Ldo_move:
	movl %edx, (%edi)

	# Advance to the next page.
	addl $(PAGE_SIZE), %esi
	addl $4, %edi
	loop 1b

	# Map the page table to page directory entry 0
	# (virtual addresses 0x00000000 to 0x003FFFFF),
	# identity mapping the kernel. This is done because
	# enabling paging does not change the address of the
	# next instruction, which continues to be physical.
	movl $(PHYS(kernel_pgtbl)), %ecx
	orl $0x00000103, %ecx
	movl %ecx, PHYS(kernel_pgdir)

	# Map the page table to page directory entry 0x300
	# (virtual addresses 0xC0000000 to 0xC03FFFFF),
	# mapping the kernel in the higher half.
	movl $(PHYS(kernel_pgtbl)), %ecx
	orl $0x00000103, %ecx
	movl %ecx, PHYS(kernel_pgdir) + 0x300 * 4

	# Map the final entry in the page directory to
	# the page directory itself.
	movl $(PHYS(kernel_pgdir)), %ecx
	orl $0x00000103, %ecx
	movl %ecx, PHYS(kernel_pgdir) + 0x3FF * 4

	# Load the physical address of the page directory into cr3.
	movl $(PHYS(kernel_pgdir)), %ecx
	movl %ecx, %cr3

	# Enable paging and the write protect bit,
	# preventing writing to read-only pages.
	movl %cr0, %ecx
	orl $0x80010000, %ecx
	movl %ecx, %cr0

#ifdef CONFIG_PAE
	leal .Lno_pae_panic, %ecx
#else
	# Jump to the higher half addresses.
	leal higher_half_start, %ecx
#endif
	jmp *%ecx

.Lno_pae_panic:
	movl $bsp_stack_top, %esp
	cld
	call _init
	call gdt_init_early
	call idt_init_early
	call vgatext_register
	push $.Lpae_panic_msg
	call panic

.section .rodata
.Lpae_panic_msg:
.ascii "improper kernel configuration for hardware\n"
.ascii "Kernel was compiled with CONFIG_PAE, but this system does not support PAE.\n"
.asciz "Please run a non-PAE x86 radix kernel.\n"

.section .text
.align 4

BEGIN_FUNC(_start)
	jmp paging_init

higher_half_start:
	# Remove the identity mapped-page and flush the TLB.
#ifdef CONFIG_PAE
	movl $0, pgdir_0
#else
	movl $0, kernel_pgdir
#endif
	invlpg (0)

	# Set stack pointer to point to the top of the stack.
	movl $bsp_stack_top, %esp
	xorl %ebp, %ebp
	cld

	# Call global constructors.
	call _init

	# Initialize basic structures for the bootstrap processor.
	call bsp_init_early

	call vgatext_register

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
END_FUNC(_start)
