.section .text
.align 4

.global tss_load
.type gdt_load, @function

tss_load:
	mov 4(%esp), %ax
	ltr %ax
	ret
