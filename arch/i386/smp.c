/*
 * arch/i386/smp.c
 * Copyright (C) 2017 Alexei Frolov
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

#include <radix/asm/apic.h>
#include <radix/asm/gdt.h>
#include <radix/asm/idt.h>

#include <radix/compiler.h>
#include <radix/cpu.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/mm.h>
#include <radix/percpu.h>
#include <radix/smp.h>

#include <rlibc/string.h>

#ifdef CONFIG_SMP

#define SMPBOOT "smpboot: "

extern int __smp_tramp_start;
extern int __smp_tramp_end;

static addr_t smp_tramp_start = (addr_t)&__smp_tramp_start;
static addr_t smp_tramp_end = (addr_t)&__smp_tramp_end;

struct gdt_desc {
	uint16_t size;
	uint32_t addr;
} __packed;

extern struct gdt_desc ap_gdt_desc;

/* initial GDT for application processors */
static uint64_t ap_gdt[] = {
	0x0000000000000000,
	0x00CF9A000000FFFF,
	0x00CF92000000FFFF
};

static struct {
	int cpu_number;
} ap_boot_info;

/*
 * arch_smp_boot:
 * Start the SMP boot sequence for x86 processors.
 * Allocate a page in the first mebibyte of physical memory,
 * copy over the AP trampoline code, and start all available
 * processers to execute this trampoline code.
 */
void arch_smp_boot(void)
{
	struct page *smp_tramp;
	size_t smp_tramp_size, gdtr_offset;
	struct gdt_desc *gd;

	if (!system_smp_capable())
		return;

	smp_tramp = alloc_page(PA_LOWMEM);
	if (IS_ERR(smp_tramp))
		panic("could not allocate memory for smp trampoline: %s\n",
		      strerror(ERR_VAL(smp_tramp)));

	smp_tramp_size = ALIGN(smp_tramp_end - smp_tramp_start, 8);
	gdtr_offset = (addr_t)&ap_gdt_desc - smp_tramp_start;

	/* identity map the trampoline page for when APs enable paging */
	map_page_kernel(virt_to_phys(smp_tramp_start),
	                virt_to_phys(smp_tramp_start),
                        PROT_WRITE, PAGE_CP_DEFAULT);

	memcpy(smp_tramp->mem, (void *)smp_tramp_start, smp_tramp_size);

	/* point the gdt descriptor to the AP GDT */
	gd = smp_tramp->mem + gdtr_offset;
	gd->size = sizeof ap_gdt;
	gd->addr = virt_to_phys(ap_gdt);

	apic_start_smp(page_to_pfn(smp_tramp));

	unmap_page(smp_tramp_start);
	free_pages(smp_tramp);
}

void prepare_ap_boot(int cpu_number)
{
	ap_boot_info.cpu_number = cpu_number;
}

void ap_switch_stack(void *stack);
void ap_stop(void);

DECLARE_PER_CPU(void *, cpu_stack);

/*
 * ap_entry:
 * Kernel entry point for application processors.
 * Load a proper GDT for the processor (with its per-CPU segment)
 * and allocate it a 16 KiB stack.
 */
void ap_entry(void)
{
	int cpu;
	struct page *p;
	void *stack_top;
	addr_t offset;

	cpu = ap_boot_info.cpu_number;
	offset = __percpu_offset[cpu];

	gdt_init_cpu(cpu, offset);
	this_cpu_write(processor_id, cpu);
	this_cpu_write(__this_cpu_offset, offset);

	p = alloc_page(PA_STANDARD);
	stack_top = p->mem + PAGE_SIZE;
	this_cpu_write(cpu_stack, stack_top);

	ap_switch_stack(stack_top);
}

/* ap_shutdown: halt the executing processor */
static void ap_shutdown(void)
{
	klog(KLOG_ERROR, SMPBOOT "shutting down processor %d", processor_id());
	ap_stop();
}

/*
 * ap_init:
 * Main application processor initialization sequence.
 */
void ap_init(void)
{
	/*
	 * Notify the BSP that the AP is active and running independently,
	 * allowing the next processor to be started.
	 * Beyond this point, multiple processors run simultaneously
	 * and synchronization is necessary.
	 */
	set_ap_active();

	read_cpu_info();

	if (cpu_init(1) != 0)
		ap_shutdown();

	if (percpu_init(1) != 0)
		ap_shutdown();

	idt_init();
}

#endif /* CONFIG_SMP */
