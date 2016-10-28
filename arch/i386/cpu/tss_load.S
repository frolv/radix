.section .text
.align 4

.global tss_load
.type tss_load, @function

tss_load:
	mov 4(%esp), %ax
	ltr %ax
	ret
