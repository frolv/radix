.section .text
.align 4

# inspired by toaruos
.macro ISR_NOERR i
isr\i:
	cli
	push $0
	push $\i
	jmp isr_common
.endm

.macro ISR_ERR i
isr\i:
	cli
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

# standard IRQs


# push all registers onto the stack as part of the struct regs
isr_common:
	# segment registers
	push %ds
	push %es
	push %fs
	push %gs

	# general purpose registers
	pusha

	call interrupt_handler

	popa

	pop %gs
	pop %fs
	pop %es
	pop %ds

	# for error code and interrupt number
	addl $8, %esp
	ret

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
	ret

.section .data

# array containing the addresses of all interrupt handlers
.global intr
intr:
	.space 128
