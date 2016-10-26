/* program entry point */
ENTRY(_start)

SECTIONS {
	/* begin at 1 MiB */
	. = 1M;

	/* place the multiboot header followed by the .text section */
	.text BLOCK(4K) : ALIGN(4K) {
		*(.multiboot)
		*(.text)
	}

	/* read-only data */
	.rodata BLOCK(4K) : ALIGN(4K) {
		*(.rodata)
	}

	/* initialized read-write data */
	.data BLOCK(4K) : ALIGN(4K) {
		*(.data)
	}

	/* uninitialized read-write data and stack */
	.bss BLOCK(4K) : ALIGN(4K) {
		*(COMMON)
		*(.bss)
	}
}
