/*
 * arch/i386/cpu/gdt.c
 * Copyright (C) 2021 Alexei Frolov
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

#include <radix/asm/gdt.h>
#include <radix/compiler.h>
#include <radix/irq.h>
#include <radix/percpu.h>

#include <string.h>

DEFINE_PER_CPU(uint64_t, gdt[8]);
DEFINE_PER_CPU(uint32_t, tss[26]);

#define GDT_SIZE (sizeof gdt)
#define TSS_SIZE (sizeof tss)

extern void gdt_load(void *base, size_t s);
extern void tss_load(uintptr_t gdt_offset);

extern uint32_t bsp_stack_top;

static void tss_init(uint32_t *tss_ptr, uint32_t esp0, uint32_t ss0);

#define GDT_ACCESSED   (1 << 0)
#define GDT_RW         (1 << 1)
#define GDT_CONFORMING (1 << 2)
#define GDT_DIRECTION  (1 << 2)
#define GDT_EXEC       (1 << 3)
#define GDT_CODEDATA   (1 << 4)
#define GDT_DPL(pl)    ((pl) << 5)
#define GDT_PRESENT    (1 << 7)

#define GDT_CODE (GDT_PRESENT | GDT_CODEDATA | GDT_EXEC | GDT_RW)
#define GDT_DATA (GDT_PRESENT | GDT_CODEDATA | GDT_RW)

#define GDT_FLAGS_64BIT (1 << 1)  // 64-bit code segment.
#define GDT_FLAGS_32BIT (1 << 2)  // 16- or 32-bit segment.
#define GDT_FLAGS_4KIB  (1 << 3)  // Byte or 4K granularity.

#define GDT_FLAGS_DEFAULT (GDT_FLAGS_4KIB | GDT_FLAGS_32BIT)

// Compresses values into a GDT descriptor.
static uint64_t gdt_entry(uint32_t base,
                          uint32_t lim,
                          uint8_t access,
                          uint8_t flags)
{
    uint64_t ret;

    /* top half of entry */
    ret = lim & 0x000F0000;
    ret |= ((uint32_t)flags << 20) & 0x00F00000;
    ret |= ((uint16_t)access << 8) & 0x0000FF00;
    ret |= (base >> 16) & 0x000000FF;
    ret |= base & 0xFF000000;

    /* bottom half of entry */
    ret <<= 32;
    ret |= lim & 0x0000FFFF;
    ret |= base << 16;

    return ret;
}

// Creates an entry in the current CPU's global descriptor table.
static __always_inline void gdt_set(
    size_t entry, uint32_t base, uint32_t lim, uint8_t access, uint8_t flags)
{
    raw_cpu_write(gdt[entry], gdt_entry(base, lim, access, flags));
}

static void __gdt_init(uint64_t *gdt_ptr, uint32_t *tss_ptr, uint32_t fsbase)
{
    uint32_t tss_base;

    tss_base = (uintptr_t)tss_ptr;

    tss_init(tss_ptr, bsp_stack_top, GDT_OFFSET(GDT_KERNEL_DATA));

    gdt_ptr[GDT_NULL] = gdt_entry(0, 0, 0, 0);
    gdt_ptr[GDT_KERNEL_CODE] =
        gdt_entry(0, 0xFFFFFFFF, GDT_CODE | GDT_DPL(0), GDT_FLAGS_DEFAULT);
    gdt_ptr[GDT_KERNEL_DATA] =
        gdt_entry(0, 0xFFFFFFFF, GDT_DATA | GDT_DPL(0), GDT_FLAGS_DEFAULT);
    gdt_ptr[GDT_USER_CODE] =
        gdt_entry(0, 0xFFFFFFFF, GDT_CODE | GDT_DPL(3), GDT_FLAGS_DEFAULT);
    gdt_ptr[GDT_USER_DATA] =
        gdt_entry(0, 0xFFFFFFFF, GDT_DATA | GDT_DPL(3), GDT_FLAGS_DEFAULT);
    gdt_ptr[GDT_TSS] =
        gdt_entry(tss_base,
                  tss_base + TSS_SIZE,
                  GDT_PRESENT | GDT_DPL(0) | GDT_EXEC | GDT_ACCESSED,
                  GDT_FLAGS_32BIT);
    gdt_ptr[GDT_FS] =
        gdt_entry(fsbase, 0xFFFFFFFF, GDT_DATA | GDT_DPL(3), GDT_FLAGS_DEFAULT);
    gdt_ptr[GDT_GS] =
        gdt_entry(0, 0xFFFFFFFF, GDT_DATA | GDT_DPL(3), GDT_FLAGS_DEFAULT);

    gdt_load(gdt_ptr, GDT_SIZE);
    tss_load(GDT_OFFSET(GDT_TSS));
}

// Populates and loads an early boot GDT for the bootstrap processor.
void gdt_init_early(void) { __gdt_init(gdt, tss, 0); }

// Populates and loads the global descriptor table for the current CPU.
void gdt_init(uint32_t fsbase)
{
    __gdt_init(raw_cpu_ptr(gdt), raw_cpu_ptr(tss), fsbase);
}

// Populates and loads the GDT belonging to the given CPU ID on the current CPU.
// Called by application processors when they boot (before they have a concept
// of processor_id(), which requires a functional GDT).
void gdt_init_cpu(int cpu, uint32_t fsbase)
{
    __gdt_init(cpu_ptr(gdt, cpu), cpu_ptr(tss, cpu), fsbase);
}

// Sets the fsbase on the current CPU during early boot. Called by the BSP
// before interrupts / percpu vars are active.
void gdt_set_initial_fsbase(uint32_t base)
{
    gdt[GDT_FS] =
        gdt_entry(base, 0xFFFFFFFF, GDT_DATA | GDT_DPL(3), GDT_FLAGS_DEFAULT);
    asm volatile("mov %0, %%fs" : : "r"(GDT_OFFSET(GDT_FS)));
}

void gdt_set_fsbase(uint32_t base)
{
    unsigned long irqstate;

    irq_save(irqstate);
    gdt_set(GDT_FS, base, 0xFFFFFFFF, GDT_DATA | GDT_DPL(3), GDT_FLAGS_DEFAULT);
    asm volatile("mov %0, %%fs" : : "r"(GDT_OFFSET(GDT_FS)));
    irq_restore(irqstate);
}

void gdt_set_gsbase(uint32_t base)
{
    unsigned long irqstate;

    irq_save(irqstate);
    gdt_set(GDT_GS, base, 0xFFFFFFFF, GDT_DATA | GDT_DPL(3), GDT_FLAGS_DEFAULT);
    asm volatile("mov %0, %%gs" : : "r"(GDT_OFFSET(GDT_GS)));
    irq_restore(irqstate);
}

void tss_set_stack(uint32_t new_esp) { this_cpu_write(tss[1], new_esp); }

// Initializes the task state segment.
//
// ESP0 is the value assigned to the stack pointer in a cross-privilege
// interrupt. SS0 holds the offset of the kernel's data segment entry in the
// GDT.
static void tss_init(uint32_t *tss_ptr, uint32_t esp0, uint32_t ss0)
{
    memset(tss_ptr, 0, TSS_SIZE);

    // ESP0 at offset 0x4.
    tss_ptr[1] = esp0;
    // SS0 at offset 0x8.
    tss_ptr[2] = ss0;
    // IOPB at offset 0x66.
    tss_ptr[25] = TSS_SIZE << 16;
}
