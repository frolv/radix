#
# arch/i386/smpboot.S
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
#include <radix/asm/page_defs.h>

.set KERNEL_VIRTUAL_BASE, 0xC0000000

.section .bootstrap_stack, "aw", @nobits
.align PAGE_SIZE
ap_stack_bottom:
.skip PAGE_SIZE
.global ap_stack_top
ap_stack_top:

.section .smp_tramp
.code16

# Application processor trampoline code.
# This initial code is relocated to a trampoline page in the first mebibyte of
# physical memory. The processor starts in real mode with the trampoline page
# in its CS segment.
BEGIN_FUNC(ap_boot)
	cli
	mov %cs, %ax
	mov %ax, %ds

	# Load a null IDT and a basic GDT which allows access to the full
	# address space using offsets from the start of the page.
	lidtl (ap_idt_desc - ap_boot)
	lgdtl (ap_gdt_desc - ap_boot)

	# enter protected mode
	mov $1, %ax
	lmsw %ax

	mov $0x10, %ax
	mov %ax, %ds

	# jump off of the trampoline page to the actual location of the SMP code
	ljmpl $0x08, $(.Lflush_segments - KERNEL_VIRTUAL_BASE)

.code32
.Lflush_segments:
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

#ifdef CONFIG_PAE
	# enable PAE
	movl %cr4, %eax
	orl $0x00000020, %eax
	movl %eax, %cr4

	movl $(kernel_pdpt - KERNEL_VIRTUAL_BASE), %ecx

#else
	movl $(kernel_pgdir - KERNEL_VIRTUAL_BASE), %ecx
#endif

	movl %ecx, %cr3

	movl %cr0, %ecx
	orl $0x80010000, %ecx
	movl %ecx, %cr0

	# jump into virtual address space
	leal .Lvirtual, %ecx
	jmp *%ecx

.Lvirtual:
	# flush the identity mapped trampoline page from the tlb
	invlpg __smp_tramp_start

	# load a temporary stack and enter the kernel proper
	movl $ap_stack_top, %esp
	xorl %ebp, %ebp
	cld
	call ap_entry
END_FUNC(ap_boot)

.align 8
ap_idt_desc:
	.word 0
	.long 0

.global ap_gdt_desc
ap_gdt_desc:
	.word 0
	.long 0


.section .text

# After a proper stack has been allocated for the AP, this function is called
# to switch to the new stack and continue the AP initialization sequence.
BEGIN_FUNC(ap_switch_stack)
	movl 4(%esp), %eax
	movl %eax, %esp
	call ap_init

1:
	sti
	hlt
	jmp 1b
END_FUNC(ap_switch_stack)
