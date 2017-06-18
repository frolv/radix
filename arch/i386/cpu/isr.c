/*
 * arch/i386/cpu/isr.c
 * Copyright (C) 2016-2017 Alexei Frolov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <radix/cpu.h>
#include <radix/error.h>
#include <radix/irq.h>
#include <radix/kernel.h>
#include <radix/regs.h>
#include <radix/task.h>

#include "apic.h"
#include "idt.h"
#include "isr.h"
#include "pic.h"

static void (*isr_vectors[])(void) = {
	isr_0, isr_1, isr_2, isr_3, isr_4, isr_5, isr_6, isr_7,
	isr_8, isr_9, isr_10, isr_11, isr_12, isr_13, isr_14, isr_15,
	isr_16, isr_17, isr_18, isr_19, isr_20, isr_21, isr_22, isr_23,
	isr_24, isr_25, isr_26, isr_27, isr_28, isr_29, isr_30, isr_31,
	isr_32, isr_33, isr_34, isr_35, isr_36, isr_37, isr_38, isr_39,
	isr_40, isr_41, isr_42, isr_43, isr_44, isr_45, isr_46, isr_47,
	isr_48, isr_49, isr_50, isr_51, isr_52, isr_53, isr_54, isr_55,
	isr_56, isr_57, isr_58, isr_59, isr_60, isr_61, isr_62, isr_63,
	isr_64, isr_65, isr_66, isr_67, isr_68, isr_69, isr_70, isr_71,
	isr_72, isr_73, isr_74, isr_75, isr_76, isr_77, isr_78, isr_79,
	isr_80, isr_81, isr_82, isr_83, isr_84, isr_85, isr_86, isr_87,
	isr_88, isr_89, isr_90, isr_91, isr_92, isr_93, isr_94, isr_95,
	isr_96, isr_97, isr_98, isr_99, isr_100, isr_101, isr_102, isr_103,
	isr_104, isr_105, isr_106, isr_107, isr_108, isr_109, isr_110, isr_111,
	isr_112, isr_113, isr_114, isr_115, isr_116, isr_117, isr_118, isr_119,
	isr_120, isr_121, isr_122, isr_123, isr_124, isr_125, isr_126, isr_127,
	isr_128, isr_129, isr_130, isr_131, isr_132, isr_133, isr_134, isr_135,
	isr_136, isr_137, isr_138, isr_139, isr_140, isr_141, isr_142, isr_143,
	isr_144, isr_145, isr_146, isr_147, isr_148, isr_149, isr_150, isr_151,
	isr_152, isr_153, isr_154, isr_155, isr_156, isr_157, isr_158, isr_159,
	isr_160, isr_161, isr_162, isr_163, isr_164, isr_165, isr_166, isr_167,
	isr_168, isr_169, isr_170, isr_171, isr_172, isr_173, isr_174, isr_175,
	isr_176, isr_177, isr_178, isr_179, isr_180, isr_181, isr_182, isr_183,
	isr_184, isr_185, isr_186, isr_187, isr_188, isr_189, isr_190, isr_191,
	isr_192, isr_193, isr_194, isr_195, isr_196, isr_197, isr_198, isr_199,
	isr_200, isr_201, isr_202, isr_203, isr_204, isr_205, isr_206, isr_207,
	isr_208, isr_209, isr_210, isr_211, isr_212, isr_213, isr_214, isr_215,
	isr_216, isr_217, isr_218, isr_219, isr_220, isr_221, isr_222, isr_223,
	isr_224, isr_225, isr_226, isr_227, isr_228, isr_229, isr_230, isr_231,
	isr_232, isr_233, isr_234, isr_235, isr_236, isr_237, isr_238, isr_239,
	isr_240, isr_241, isr_242, isr_243, isr_244, isr_245, isr_246, isr_247,
	isr_248, isr_249, isr_250, isr_251, isr_252, isr_253, isr_254, isr_255
};

extern void isr_table_setup(void);

/* cpu exception handler functions */
static void (*exception_handlers[NUM_EXCEPTIONS])(struct regs *, int);

/* hardware interrupt handler functions */
static void (*irq_handlers[NUM_ISR_VECTORS])(struct regs *);

static void debug_pf(struct regs *r, int errno)
{
	unsigned long addr;

	asm volatile("movl %%cr2, %0" : "=r"(addr));
	panic("page fault at address 0x%08lX\n", addr);

	(void)r;
	(void)errno;
}

void load_interrupt_routines(void)
{
	size_t i;

	/* add exception routines */
	for (i = 0; i < NUM_ISR_VECTORS; ++i)
		idt_set(i, (uintptr_t)isr_vectors[i], 0x08, 0x8E);

	if (cpu_supports(CPUID_APIC | CPUID_MSR) && apic_parse_madt() == 0) {
		/* APIC is available; use it. */
		apic_init();
	}

	install_exception_handler(0x0E, debug_pf);
}

/* install_exception_handler: set a function to handle exception `intno` */
int install_exception_handler(uint32_t intno, void (*hnd)(struct regs *, int))
{
	if (intno >= IRQ_BASE)
		return EINVAL;

	exception_handlers[intno] = hnd;
	return 0;
}

/* uninstall_exception_handler: set a function to handle exception `intno` */
int uninstall_exception_handler(uint32_t intno)
{
	if (intno >= IRQ_BASE)
		return EINVAL;

	exception_handlers[intno] = NULL;
	return 0;
}

/* install_interrupt_handler: set a function to handle IRQ `intno` */
int install_interrupt_handler(uint32_t intno, void (*hnd)(struct regs *))
{
	if (intno < IRQ_BASE || intno >= NUM_ISR_VECTORS)
		return EINVAL;

	irq_handlers[intno] = hnd;
	return 0;
}

/* uninstall_interrupt_handler: remove the handler function for `intno` */
int uninstall_interrupt_handler(uint32_t intno)
{
	if (intno < IRQ_BASE || intno >= NUM_ISR_VECTORS)
		return EINVAL;

	irq_handlers[intno] = NULL;
	return 0;
}

/* interrupt_disable: disable IRQs */
void interrupt_disable(void)
{
	struct task *curr;

	asm volatile("cli");

	curr = current_task();
	if (likely(curr))
		curr->interrupt_depth++;
}

/* interrupt_enable: enable IRQs */
void interrupt_enable(void)
{
	struct task *curr;

	curr = current_task();
	if (likely(curr)) {
		if (curr->interrupt_depth)
			curr->interrupt_depth--;

		if (!curr->interrupt_depth)
			asm volatile("sti");
	} else {
		asm volatile("sti");
	}
}

static const char *exceptions[] = {
	"Division by zero",                     /* 0x00 */
	"Debug",                                /* 0x01 */
	"Non-maskable interrupt",               /* 0x02 */
	"Breakpoint",                           /* 0x03 */
	"Overflow",                             /* 0x04 */
	"Out of bounds",                        /* 0x05 */
	"Invalid opcode",                       /* 0x06 */
	"Device not available",                 /* 0x07 */
	"Double fault",                         /* 0x08 */
	"Coprocessor segment overrun",          /* 0x09 */
	"Invalid TSS",                          /* 0x0A */
	"Segment not present",                  /* 0x0B */
	"Stack fault",                          /* 0x0C */
	"General protection fault",             /* 0x0D */
	"Page fault",                           /* 0x0E */
	"Unknown exception",                    /* 0x0F */
	"x87 floating-point exception",         /* 0x10 */
	"Alignment check",                      /* 0x11 */
	"Machine check",                        /* 0x12 */
	"SIMD floating-point exception",        /* 0x13 */
	"Virtualization exception",             /* 0x14 */
	"Unknown exception",                    /* 0x15 */
	"Unknown exception",                    /* 0x16 */
	"Unknown exception",                    /* 0x17 */
	"Unknown exception",                    /* 0x18 */
	"Unknown exception",                    /* 0x19 */
	"Unknown exception",                    /* 0x1A */
	"Unknown exception",                    /* 0x1B */
	"Unknown exception",                    /* 0x1C */
	"Unknown exception",                    /* 0x1D */
	"Security exception",                   /* 0x1E */
	"Unknown exception"                     /* 0x1F */
};

/* TODO: with multiprocessing, this needs to exist per CPU */
static int interrupt = 0;

/*
 * interrupt_handler:
 * Common interrupt handler. Saves registers and calls handler
 * for specific interrupt.
 */
void interrupt_handler(struct interrupt_regs ir)
{
	struct regs r;

	interrupt = 1;
	save_registers(&ir, &r);

	if (ir.intno < IRQ_BASE) {
		if (exception_handlers[ir.intno])
			exception_handlers[ir.intno](&r, ir.errno);
		else
			panic("unhandled CPU exception 0x%02X `%s'\n",
			      ir.intno, exceptions[ir.intno]);
	} else {
		/* TODO: send EOI if necessary */
		if (irq_handlers[ir.intno])
			irq_handlers[ir.intno](&r);
	}

	load_registers(&ir, &r);
	interrupt = 0;
}

int in_interrupt(void)
{
	return interrupt;
}
