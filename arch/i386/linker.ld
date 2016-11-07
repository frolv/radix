/* program entry point */
ENTRY(_start)

SECTIONS {
	/* begin at 3 GiB + 1 MiB */
	. = 0xC0100000;

	_kernel_start = .;

	/* place the multiboot header followed by the .text section */
	.text ALIGN (4K) : AT (ADDR(.text) - 0xC0000000)
	{
		*(.multiboot)
		*(.text)
	}

	/* read-only data */
	.rodata ALIGN (4K) : AT (ADDR(.rodata) - 0xC0000000)
	{
		*(.rodata)
	}

	/* initialized read-write data */
	.data ALIGN (4K) : AT (ADDR(.data) - 0xC0000000)
	{
		*(.data)
	}

	/* uninitialized read-write data and stack */
	.bss ALIGN (4K) : AT (ADDR(.bss) - 0xC0000000)
	{
		*(COMMON)
		*(.bss)
		*(.bootstrap_stack)
	}

	_kernel_end = .;
}
