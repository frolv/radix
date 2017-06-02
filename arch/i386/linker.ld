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

	_kernel_start = .;

	.text ALIGN (4K) : AT (ADDR(.text) - 0xC0000000) {
		*(.multiboot)
		*(.text)
	} :text

	/* read-only data */
	.rodata ALIGN (4K) : AT (ADDR(.rodata) - 0xC0000000) {
		*(.rodata)
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
		__per_cpu_start = .;
		*(.percpu_data)
		__per_cpu_end = .;
	} :readonly

	_kernel_end = .;
}
