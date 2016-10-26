/* constants for multiboot header */
.set ALIGN,	1 << 0
.set MEMINFO,	1 << 1
.set FLAGS,	ALIGN | MEMINFO
.set MAGIC,	0x1BADB002
.set CHECKSUM,	-(MAGIC + FLAGS)

/* Declare a multiboot header that marks the program as a kernel. */
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/*
 * Allocate room for a small stack by creating a symbol at the bottom,
 * skipping ahead 16 KiB and creating a symbol at the top. The stack
 * must be 16-byte aligned on x86.
 */
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

.section .text
.global _start
.type _start, @function
/* kernel entry point */
_start:
	/* set the stack pointer to the top of the stack */
	mov $stack_top, %esp

	/* call global constructors */
	call _init

	/*
	 * The ABI requires that the stack is 16-byte aligned
	 * at the time of the call instruction. As we our stack
	 * was initially 16-byte aligned and we have not pushed
	 * anything onto it, this call is well defined.
	 */
	call kmain

	/* Hang if kmain returns. */
	cli
1:	hlt
	jmp 1b

/* set the size of _start symbol to the current location '.' minus its start */
.size _start, . - _start
