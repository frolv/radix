.section .text
.align 4

#include <radix/assembler.h>

BEGIN_FUNC(tss_load)
	mov 4(%esp), %ax
	ltr %ax
	ret
END_FUNC(tss_load)
