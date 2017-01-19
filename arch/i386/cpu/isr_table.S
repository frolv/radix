.section .text
.align 4

# inspired by toaruos
.macro ISR_NOERR i
isr\i:
	push $0
	push $\i
	jmp isr_common
.endm

.macro ISR_ERR i
isr\i:
	push $\i
	jmp isr_common
.endm

.macro IRQ i
irq\i:
	push $0
	push $(\i + 32)
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

# standard IRQs
IRQ 0
IRQ 1
IRQ 2
IRQ 3
IRQ 4
IRQ 5
IRQ 6
IRQ 7
IRQ 8
IRQ 9
IRQ 10
IRQ 11
IRQ 12
IRQ 13
IRQ 14
IRQ 15

# system call interrupt
sysc:
	push $0
	push $0x80
	jmp isr_common

isr_common:
	testw $0x3, 12(%esp)
	jz stack_fix

# Push all registers onto the stack as part of a struct regs.
# The order in which these are pushed is important; see
# arch/i386/include/untitled/sys.h for the struct definition.
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
	pop %fs
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

.global isr_table_setup
# load the addresses of all interrupt functions into the intr array
isr_table_setup:
	movl $intr,  %eax
	movl $isr0,  0(%eax)
	movl $isr1,  4(%eax)
	movl $isr2,  8(%eax)
	movl $isr3,  12(%eax)
	movl $isr4,  16(%eax)
	movl $isr5,  20(%eax)
	movl $isr6,  24(%eax)
	movl $isr7,  28(%eax)
	movl $isr8,  32(%eax)
	movl $isr9,  36(%eax)
	movl $isr10, 40(%eax)
	movl $isr11, 44(%eax)
	movl $isr12, 48(%eax)
	movl $isr13, 52(%eax)
	movl $isr14, 56(%eax)
	movl $isr15, 60(%eax)
	movl $isr16, 64(%eax)
	movl $isr17, 68(%eax)
	movl $isr18, 72(%eax)
	movl $isr19, 76(%eax)
	movl $isr20, 80(%eax)
	movl $isr21, 84(%eax)
	movl $isr22, 88(%eax)
	movl $isr23, 92(%eax)
	movl $isr24, 96(%eax)
	movl $isr25, 100(%eax)
	movl $isr26, 104(%eax)
	movl $isr27, 108(%eax)
	movl $isr28, 112(%eax)
	movl $isr29, 116(%eax)
	movl $isr30, 120(%eax)
	movl $isr31, 124(%eax)
	movl $irq0,  128(%eax)
	movl $irq1,  132(%eax)
	movl $irq2,  136(%eax)
	movl $irq3,  140(%eax)
	movl $irq4,  144(%eax)
	movl $irq5,  148(%eax)
	movl $irq6,  152(%eax)
	movl $irq7,  156(%eax)
	movl $irq8,  160(%eax)
	movl $irq9,  164(%eax)
	movl $irq10, 168(%eax)
	movl $irq11, 172(%eax)
	movl $irq12, 176(%eax)
	movl $irq13, 180(%eax)
	movl $irq14, 184(%eax)
	movl $irq15, 188(%eax)
	movl $sysc,  192(%eax)
	ret

.section .data

# array containing the addresses of all interrupt handlers
.global intr
intr:
	.space 196
