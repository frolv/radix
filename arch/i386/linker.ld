/* program entry point */
ENTRY(_start)

PHDRS {
	text PT_LOAD FLAGS(5);
	readonly PT_LOAD FLAGS(4);
	data PT_LOAD FLAGS(6);
}

SECTIONS {
	/* begin at 3 GiB + 1 MiB */
	. = 0xC0100000;

	__kernel_start = .;

	.text ALIGN (4K) : AT (ADDR(.text) - 0xC0000000) {
		__text_start = .;
		*(.multiboot)
		*(.text)
		. = ALIGN(4K);
		__smp_tramp_start = .;
		*(.smp_tramp)
		__smp_tramp_end = .;
		*(.init)
		*(.fini)
		. = ALIGN(4K);
		__text_end = .;
	} :text

	/* read-only data */
	.rodata ALIGN (4K) : AT (ADDR(.rodata) - 0xC0000000) {
		__rodata_start = .;
		*(.rodata)
		*(SORT(.rodata.*))
		. = ALIGN(4K);
		__rodata_end = .;
	} :readonly

	/* initialized read-write data */
	.data ALIGN (4K) : AT (ADDR(.data) - 0xC0000000) {
		*(.data)
	} :data

	/* uninitialized read-write data and stack */
	.bss ALIGN (4K) : AT (ADDR(.bss) - 0xC0000000) {
		*(COMMON)
		*(.bss)
		*(.bootstrap_stack)
	} :data

	.percpu_data ALIGN (4K) : AT (ADDR(.percpu_data) - 0xC0000000) {
		__percpu_start = .;
		*(.percpu_data)
		__percpu_end = .;
	} :data

	. = ALIGN(4K);

	__kernel_end = .;
}
