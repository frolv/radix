/*
 * arch/i386/cpu/cpu.c
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

#include <radix/asm/apic.h>
#include <radix/asm/gdt.h>
#include <radix/asm/idt.h>
#include <radix/asm/pat.h>
#include <radix/cpu.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/percpu.h>
#include <radix/smp.h>

#include <rlibc/stdio.h>
#include <rlibc/string.h>

struct cpu_cache {
    /* id[0..3]: level; id[4..7]: type */
    unsigned char id;
    unsigned char associativity;
    unsigned short line_size;
    unsigned long size;
};

#define MAX_CACHES 10

struct cache_info {
    struct cpu_cache caches[MAX_CACHES];
    unsigned int ncaches;
    unsigned long line_size;
    unsigned long prefetching;

    /* Instruction TLB */
    unsigned long tlbi_page_size;
    unsigned long tlbi_entries;
    unsigned long tlbi_assoc;

    /* Data TLB */
    unsigned long tlbd_page_size;
    unsigned long tlbd_entries;
    unsigned long tlbd_assoc;
};

enum {
    CACHE_ASSOC_FULL = 1,
    CACHE_ASSOC_2WAY = 2,
    CACHE_ASSOC_4WAY = 4,
    CACHE_ASSOC_6WAY = 6,
    CACHE_ASSOC_8WAY = 8,
    CACHE_ASSOC_12WAY = 12,
    CACHE_ASSOC_16WAY = 16,
    CACHE_ASSOC_24WAY = 24
};

/* Same as cpuid 0x4 cache type values. */
enum { CACHE_TYPE_DATA = 1, CACHE_TYPE_INSTRUCTION, CACHE_TYPE_UNIFIED };

struct cpu_info {
    long cpuid_max;
    char vendor_id[16];
    unsigned long cpuid_1[4];
    uint64_t cpu_features;
    struct cache_info cache_info;
};

static DEFINE_PER_CPU(struct cpu_info, cpu_info);

/* CPU features shared across all active CPUs */
static uint64_t cpu_shared_features;
/* BSP's cache line size; used globally across all CPUs */
static int cache_line_size;

#define PAGE_SIZE_4K   (1 << 0)
#define PAGE_SIZE_2M   (1 << 1)
#define PAGE_SIZE_4M   (1 << 2)
#define PAGE_SIZE_256M (1 << 3)
#define PAGE_SIZE_1G   (1 << 4)

static void add_cache(unsigned char level,
                      unsigned char type,
                      unsigned long size,
                      unsigned long line_size,
                      unsigned long assoc);
static void set_tlb_info(int which,
                         unsigned long page_size,
                         unsigned long entries,
                         unsigned long assoc);
static int read_cache_info(void);
static void extended_processor_info(void);

/*
 * read_cpu_info:
 * Use cpuid to determine information about the current processor.
 */
void read_cpu_info(void)
{
    uint64_t cpu_features;
    unsigned long cpuid_max;
    unsigned long *cpu_buf;
    unsigned long buf[4];

    if (!cpuid_supported()) {
        this_cpu_write(cpu_info.vendor_id[0], '\0');
        this_cpu_write(cpu_info.cpuid_max, 0);
        cache_line_size = 32;
        return;
    }

    cpu_buf = (unsigned long *)this_cpu_ptr(&cpu_info.vendor_id);
    cpuid(0, cpuid_max, buf[0], buf[2], buf[1]);
    buf[3] = 0;
    memcpy(cpu_buf, buf, sizeof buf);

    /* store CPUID 1 information */
    cpu_buf = (unsigned long *)this_cpu_ptr(&cpu_info.cpuid_1);
    cpuid(1, cpu_buf[0], cpu_buf[1], cpu_buf[2], cpu_buf[3]);
    cpu_features = ((uint64_t)cpu_buf[2] << 32) | cpu_buf[3];
    this_cpu_write(cpu_info.cpuid_max, cpuid_max);
    this_cpu_write(cpu_info.cpu_features, cpu_features);

    if (!cpu_shared_features)
        cpu_shared_features = cpu_features;
    else
        cpu_shared_features &= cpu_features;

    if (cpuid_max < 2 || read_cache_info() != 0) {
        /*
         * If cache information cannot be read, assume
         * default values from Intel Pentium P5.
         */
        add_cache(1, CACHE_TYPE_DATA, KIB(8), 32, CACHE_ASSOC_2WAY);
        add_cache(1, CACHE_TYPE_INSTRUCTION, KIB(8), 32, CACHE_ASSOC_4WAY);
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(256), 32, CACHE_ASSOC_4WAY);
        this_cpu_write(cpu_info.cache_info.line_size, 32);
        if (!cache_line_size)
            cache_line_size = 32;

        set_tlb_info(
            CACHE_TYPE_INSTRUCTION, PAGE_SIZE_4K, 32, CACHE_ASSOC_4WAY);
        set_tlb_info(CACHE_TYPE_DATA, PAGE_SIZE_4K, 64, CACHE_ASSOC_4WAY);
    }

    cpu_cache_str();
    extended_processor_info();
}

unsigned long i386_cache_line_size(void) { return cache_line_size; }

void i386_set_kernel_stack(void *stack) { tss_set_stack((unsigned long)stack); }

int cpu_supports(uint64_t features)
{
    return !!(cpu_shared_features & features);
}

static void add_cache(unsigned char level,
                      unsigned char type,
                      unsigned long size,
                      unsigned long line_size,
                      unsigned long assoc)
{
    int ncaches;

    ncaches = this_cpu_read(cpu_info.cache_info.ncaches);
    if (ncaches == MAX_CACHES)
        return;

    this_cpu_write(cpu_info.cache_info.caches[ncaches].id, level | (type << 4));
    this_cpu_write(cpu_info.cache_info.caches[ncaches].associativity, assoc);
    this_cpu_write(cpu_info.cache_info.caches[ncaches].line_size, line_size);
    this_cpu_write(cpu_info.cache_info.caches[ncaches].size, size);
    this_cpu_write(cpu_info.cache_info.ncaches, ncaches + 1);
}

static void read_cpuid4(void);

static void set_cache_line_size(void)
{
    int line_size, i;
    unsigned int id;

    /* set overall line size to line size of L1 cache */
    i = 0;
    while (1) {
        id = this_cpu_read(cpu_info.cache_info.caches[i].id);
        if ((id & 0xF) == 1)
            break;
        ++i;
    }

    line_size = this_cpu_read(cpu_info.cache_info.caches[i].line_size);
    this_cpu_write(cpu_info.cache_info.line_size, line_size);
    if (!cache_line_size)
        cache_line_size = line_size;
}

static void set_tlb_info(int which,
                         unsigned long page_size,
                         unsigned long entries,
                         unsigned long assoc)
{
    if (which == CACHE_TYPE_INSTRUCTION) {
        this_cpu_write(cpu_info.cache_info.tlbi_page_size, page_size);
        this_cpu_write(cpu_info.cache_info.tlbi_entries, entries);
        this_cpu_write(cpu_info.cache_info.tlbi_assoc, assoc);
    } else {
        this_cpu_write(cpu_info.cache_info.tlbd_page_size, page_size);
        this_cpu_write(cpu_info.cache_info.tlbd_entries, entries);
        this_cpu_write(cpu_info.cache_info.tlbd_assoc, assoc);
    }
}

static void process_cpuid2_descriptor(int desc)
{
    switch (desc) {
    case 0x00:
        /* null descriptor */
        break;
    case 0x01:
        set_tlb_info(
            CACHE_TYPE_INSTRUCTION, PAGE_SIZE_4K, 32, CACHE_ASSOC_4WAY);
        break;
    case 0x02:
        set_tlb_info(CACHE_TYPE_INSTRUCTION, PAGE_SIZE_4M, 2, CACHE_ASSOC_FULL);
        break;
    case 0x03:
        set_tlb_info(CACHE_TYPE_DATA, PAGE_SIZE_4K, 64, CACHE_ASSOC_4WAY);
        break;
    case 0x04:
        set_tlb_info(CACHE_TYPE_DATA, PAGE_SIZE_4M, 8, CACHE_ASSOC_4WAY);
        break;
    case 0x05:
        set_tlb_info(CACHE_TYPE_DATA, PAGE_SIZE_4M, 32, CACHE_ASSOC_4WAY);
        break;
    case 0x06:
        add_cache(1, CACHE_TYPE_INSTRUCTION, KIB(8), 32, CACHE_ASSOC_2WAY);
        break;
    case 0x08:
        add_cache(1, CACHE_TYPE_INSTRUCTION, KIB(16), 32, CACHE_ASSOC_4WAY);
        break;
    case 0x09:
        add_cache(1, CACHE_TYPE_INSTRUCTION, KIB(32), 64, CACHE_ASSOC_4WAY);
        break;
    case 0x0A:
        add_cache(1, CACHE_TYPE_DATA, KIB(8), 32, CACHE_ASSOC_2WAY);
        break;
    case 0x0B:
        set_tlb_info(CACHE_TYPE_INSTRUCTION, PAGE_SIZE_4M, 4, CACHE_ASSOC_4WAY);
        break;
    case 0x0C:
        add_cache(1, CACHE_TYPE_DATA, KIB(16), 32, CACHE_ASSOC_4WAY);
        break;
    case 0x0D:
        add_cache(1, CACHE_TYPE_DATA, KIB(16), 64, CACHE_ASSOC_4WAY);
        break;
    case 0x0E:
        add_cache(1, CACHE_TYPE_DATA, KIB(24), 64, CACHE_ASSOC_6WAY);
        break;
    case 0x10:
        add_cache(1, CACHE_TYPE_DATA, KIB(16), 32, CACHE_ASSOC_4WAY);
        break;
    case 0x15:
        add_cache(1, CACHE_TYPE_INSTRUCTION, KIB(16), 32, CACHE_ASSOC_4WAY);
        break;
    case 0x1A:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(96), 64, CACHE_ASSOC_6WAY);
        break;
    case 0x1D:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(128), 64, CACHE_ASSOC_2WAY);
        break;
    case 0x21:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(256), 64, CACHE_ASSOC_8WAY);
        break;
    case 0x22:
        add_cache(3, CACHE_TYPE_UNIFIED, KIB(512), 64, CACHE_ASSOC_4WAY);
        break;
    case 0x23:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(1), 64, CACHE_ASSOC_8WAY);
        break;
    case 0x24:
        add_cache(2, CACHE_TYPE_UNIFIED, MIB(1), 64, CACHE_ASSOC_16WAY);
        break;
    case 0x25:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(2), 64, CACHE_ASSOC_8WAY);
        break;
    case 0x29:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(4), 64, CACHE_ASSOC_8WAY);
        break;
    case 0x2C:
        add_cache(1, CACHE_TYPE_DATA, KIB(32), 64, CACHE_ASSOC_8WAY);
        break;
    case 0x30:
        add_cache(1, CACHE_TYPE_INSTRUCTION, KIB(32), 64, CACHE_ASSOC_8WAY);
        break;
    case 0x39:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(128), 64, CACHE_ASSOC_4WAY);
        break;
    case 0x3A:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(192), 64, CACHE_ASSOC_6WAY);
        break;
    case 0x3B:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(128), 64, CACHE_ASSOC_2WAY);
        break;
    case 0x3C:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(256), 64, CACHE_ASSOC_4WAY);
        break;
    case 0x3D:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(384), 64, CACHE_ASSOC_6WAY);
        break;
    case 0x3E:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(512), 64, CACHE_ASSOC_4WAY);
        break;
    case 0x41:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(128), 32, CACHE_ASSOC_4WAY);
        break;
    case 0x42:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(256), 32, CACHE_ASSOC_4WAY);
        break;
    case 0x43:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(512), 32, CACHE_ASSOC_4WAY);
        break;
    case 0x44:
        add_cache(2, CACHE_TYPE_UNIFIED, MIB(1), 32, CACHE_ASSOC_4WAY);
        break;
    case 0x45:
        add_cache(2, CACHE_TYPE_UNIFIED, MIB(2), 32, CACHE_ASSOC_4WAY);
        break;
    case 0x46:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(4), 64, CACHE_ASSOC_4WAY);
        break;
    case 0x47:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(8), 64, CACHE_ASSOC_8WAY);
        break;
    case 0x48:
        add_cache(2, CACHE_TYPE_UNIFIED, MIB(3), 64, CACHE_ASSOC_12WAY);
        break;
    case 0x49:
        /* TODO: L3 on P4, L2 on Core 2 */
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(4), 64, CACHE_ASSOC_16WAY);
        break;
    case 0x4A:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(6), 64, CACHE_ASSOC_12WAY);
        break;
    case 0x4B:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(8), 64, CACHE_ASSOC_16WAY);
        break;
    case 0x4C:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(12), 64, CACHE_ASSOC_12WAY);
        break;
    case 0x4D:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(16), 64, CACHE_ASSOC_16WAY);
        break;
    case 0x4E:
        add_cache(2, CACHE_TYPE_UNIFIED, MIB(6), 64, CACHE_ASSOC_24WAY);
        break;
    case 0x4F:
        set_tlb_info(
            CACHE_TYPE_INSTRUCTION, PAGE_SIZE_4K, 32, CACHE_ASSOC_FULL);
        break;
    case 0x50:
        set_tlb_info(CACHE_TYPE_INSTRUCTION,
                     PAGE_SIZE_4K | PAGE_SIZE_2M | PAGE_SIZE_4M,
                     32,
                     CACHE_ASSOC_FULL);
        break;
    case 0x51:
        set_tlb_info(CACHE_TYPE_INSTRUCTION,
                     PAGE_SIZE_4K | PAGE_SIZE_2M | PAGE_SIZE_4M,
                     128,
                     CACHE_ASSOC_FULL);
        break;
    case 0x52:
        set_tlb_info(CACHE_TYPE_INSTRUCTION,
                     PAGE_SIZE_4K | PAGE_SIZE_2M | PAGE_SIZE_4M,
                     256,
                     CACHE_ASSOC_FULL);
        break;
    case 0x55:
        set_tlb_info(CACHE_TYPE_INSTRUCTION,
                     PAGE_SIZE_2M | PAGE_SIZE_4M,
                     7,
                     CACHE_ASSOC_FULL);
        break;
    case 0x56:
        set_tlb_info(CACHE_TYPE_DATA, PAGE_SIZE_4M, 16, CACHE_ASSOC_4WAY);
        break;
    case 0x57:
        set_tlb_info(CACHE_TYPE_DATA, PAGE_SIZE_4K, 16, CACHE_ASSOC_4WAY);
        break;
    case 0x59:
        set_tlb_info(CACHE_TYPE_DATA, PAGE_SIZE_4K, 16, CACHE_ASSOC_FULL);
        break;
    case 0x5A:
        set_tlb_info(
            CACHE_TYPE_DATA, PAGE_SIZE_2M | PAGE_SIZE_4M, 32, CACHE_ASSOC_4WAY);
        break;
    case 0x5B:
        set_tlb_info(
            CACHE_TYPE_DATA, PAGE_SIZE_4K | PAGE_SIZE_4M, 64, CACHE_ASSOC_FULL);
        break;
    case 0x5C:
        set_tlb_info(CACHE_TYPE_DATA,
                     PAGE_SIZE_4K | PAGE_SIZE_4M,
                     128,
                     CACHE_ASSOC_FULL);
        break;
    case 0x5D:
        set_tlb_info(CACHE_TYPE_DATA,
                     PAGE_SIZE_4K | PAGE_SIZE_4M,
                     256,
                     CACHE_ASSOC_FULL);
        break;
    case 0x60:
        add_cache(1, CACHE_TYPE_DATA, KIB(16), 64, CACHE_ASSOC_4WAY);
        break;
    case 0x61:
        set_tlb_info(
            CACHE_TYPE_INSTRUCTION, PAGE_SIZE_4K, 48, CACHE_ASSOC_FULL);
        break;
    case 0x63:
        set_tlb_info(
            CACHE_TYPE_DATA, PAGE_SIZE_2M | PAGE_SIZE_4M, 32, CACHE_ASSOC_4WAY);
        break;
    case 0x64:
        set_tlb_info(CACHE_TYPE_DATA, PAGE_SIZE_4K, 512, CACHE_ASSOC_4WAY);
        break;
    case 0x66:
        add_cache(1, CACHE_TYPE_DATA, KIB(8), 64, CACHE_ASSOC_4WAY);
        break;
    case 0x67:
        add_cache(1, CACHE_TYPE_DATA, KIB(16), 64, CACHE_ASSOC_4WAY);
        break;
    case 0x68:
        add_cache(1, CACHE_TYPE_DATA, KIB(32), 64, CACHE_ASSOC_4WAY);
        break;
    case 0x6A:
        set_tlb_info(CACHE_TYPE_DATA, PAGE_SIZE_4K, 64, CACHE_ASSOC_8WAY);
        break;
    case 0x6B:
        set_tlb_info(CACHE_TYPE_DATA, PAGE_SIZE_4K, 256, CACHE_ASSOC_8WAY);
        break;
    case 0x6C:
        set_tlb_info(CACHE_TYPE_DATA,
                     PAGE_SIZE_2M | PAGE_SIZE_4M,
                     126,
                     CACHE_ASSOC_8WAY);
        break;
    case 0x6D:
        set_tlb_info(CACHE_TYPE_DATA, PAGE_SIZE_1G, 16, CACHE_ASSOC_FULL);
        break;
    case 0x76:
        set_tlb_info(CACHE_TYPE_INSTRUCTION,
                     PAGE_SIZE_2M | PAGE_SIZE_4M,
                     8,
                     CACHE_ASSOC_FULL);
        break;
    case 0x77:
        add_cache(1, CACHE_TYPE_INSTRUCTION, KIB(16), 64, CACHE_ASSOC_4WAY);
        break;
    case 0x78:
        add_cache(2, CACHE_TYPE_UNIFIED, MIB(1), 64, CACHE_ASSOC_4WAY);
        break;
    case 0x79:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(128), 64, CACHE_ASSOC_8WAY);
        break;
    case 0x7A:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(256), 64, CACHE_ASSOC_8WAY);
        break;
    case 0x7B:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(512), 64, CACHE_ASSOC_8WAY);
        break;
    case 0x7C:
        add_cache(2, CACHE_TYPE_UNIFIED, MIB(1), 64, CACHE_ASSOC_8WAY);
        break;
    case 0x7D:
        add_cache(2, CACHE_TYPE_UNIFIED, MIB(2), 64, CACHE_ASSOC_8WAY);
        break;
    case 0x7E:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(256), 128, CACHE_ASSOC_8WAY);
        break;
    case 0x7F:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(512), 64, CACHE_ASSOC_2WAY);
        break;
    case 0x80:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(512), 64, CACHE_ASSOC_8WAY);
        break;
    case 0x81:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(128), 32, CACHE_ASSOC_8WAY);
        break;
    case 0x82:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(256), 32, CACHE_ASSOC_8WAY);
        break;
    case 0x83:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(512), 32, CACHE_ASSOC_8WAY);
        break;
    case 0x84:
        add_cache(2, CACHE_TYPE_UNIFIED, MIB(1), 32, CACHE_ASSOC_8WAY);
        break;
    case 0x85:
        add_cache(2, CACHE_TYPE_UNIFIED, MIB(2), 32, CACHE_ASSOC_8WAY);
        break;
    case 0x86:
        add_cache(2, CACHE_TYPE_UNIFIED, KIB(512), 64, CACHE_ASSOC_4WAY);
        break;
    case 0x87:
        add_cache(2, CACHE_TYPE_UNIFIED, MIB(1), 64, CACHE_ASSOC_8WAY);
        break;
    case 0x88:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(2), 64, CACHE_ASSOC_4WAY);
        break;
    case 0x89:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(4), 64, CACHE_ASSOC_4WAY);
        break;
    case 0x8A:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(8), 64, CACHE_ASSOC_4WAY);
        break;
    case 0x8D:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(3), 128, CACHE_ASSOC_12WAY);
        break;
    case 0x90:
        set_tlb_info(CACHE_TYPE_INSTRUCTION,
                     PAGE_SIZE_4K | PAGE_SIZE_256M,
                     64,
                     CACHE_ASSOC_FULL);
        break;
    case 0x96:
        set_tlb_info(CACHE_TYPE_DATA,
                     PAGE_SIZE_4K | PAGE_SIZE_256M,
                     32,
                     CACHE_ASSOC_FULL);
        break;
    case 0xA0:
        set_tlb_info(CACHE_TYPE_DATA, PAGE_SIZE_4K, 32, CACHE_ASSOC_FULL);
        break;
    case 0xB0:
        set_tlb_info(
            CACHE_TYPE_INSTRUCTION, PAGE_SIZE_4K, 128, CACHE_ASSOC_4WAY);
        break;
    case 0xB1:
        set_tlb_info(CACHE_TYPE_INSTRUCTION, PAGE_SIZE_4M, 4, CACHE_ASSOC_4WAY);
        break;
    case 0xB2:
        set_tlb_info(
            CACHE_TYPE_INSTRUCTION, PAGE_SIZE_4K, 64, CACHE_ASSOC_4WAY);
        break;
    case 0xB3:
        set_tlb_info(CACHE_TYPE_DATA, PAGE_SIZE_4K, 128, CACHE_ASSOC_4WAY);
        break;
    case 0xB4:
        set_tlb_info(CACHE_TYPE_DATA, PAGE_SIZE_4K, 256, CACHE_ASSOC_4WAY);
        break;
    case 0xB5:
        set_tlb_info(
            CACHE_TYPE_INSTRUCTION, PAGE_SIZE_4K, 64, CACHE_ASSOC_8WAY);
        break;
    case 0xB6:
        set_tlb_info(
            CACHE_TYPE_INSTRUCTION, PAGE_SIZE_4K, 128, CACHE_ASSOC_8WAY);
        break;
    case 0xBA:
        set_tlb_info(CACHE_TYPE_DATA, PAGE_SIZE_4K, 64, CACHE_ASSOC_4WAY);
        break;
    case 0xC0:
        set_tlb_info(
            CACHE_TYPE_DATA, PAGE_SIZE_4K | PAGE_SIZE_4M, 8, CACHE_ASSOC_4WAY);
        break;
    case 0xC2:
        set_tlb_info(
            CACHE_TYPE_DATA, PAGE_SIZE_2M | PAGE_SIZE_4M, 16, CACHE_ASSOC_4WAY);
        break;
    case 0xC4:
        set_tlb_info(
            CACHE_TYPE_DATA, PAGE_SIZE_2M | PAGE_SIZE_4M, 32, CACHE_ASSOC_4WAY);
        break;
    case 0xD0:
        add_cache(3, CACHE_TYPE_UNIFIED, KIB(512), 64, CACHE_ASSOC_4WAY);
        break;
    case 0xD1:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(1), 64, CACHE_ASSOC_4WAY);
        break;
    case 0xD2:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(2), 64, CACHE_ASSOC_4WAY);
        break;
    case 0xD6:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(1), 64, CACHE_ASSOC_8WAY);
        break;
    case 0xD7:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(2), 64, CACHE_ASSOC_8WAY);
        break;
    case 0xD8:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(4), 64, CACHE_ASSOC_8WAY);
        break;
    case 0xDC:
        add_cache(3, CACHE_TYPE_UNIFIED, KIB(1536), 64, CACHE_ASSOC_12WAY);
        break;
    case 0xDD:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(3), 64, CACHE_ASSOC_12WAY);
        break;
    case 0xDE:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(6), 64, CACHE_ASSOC_12WAY);
        break;
    case 0xE2:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(2), 64, CACHE_ASSOC_16WAY);
        break;
    case 0xE3:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(4), 64, CACHE_ASSOC_16WAY);
        break;
    case 0xE4:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(8), 64, CACHE_ASSOC_16WAY);
        break;
    case 0xEA:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(12), 64, CACHE_ASSOC_24WAY);
        break;
    case 0xEB:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(18), 64, CACHE_ASSOC_24WAY);
        break;
    case 0xEC:
        add_cache(3, CACHE_TYPE_UNIFIED, MIB(24), 64, CACHE_ASSOC_24WAY);
        break;
    case 0xF0:
        this_cpu_write(cpu_info.cache_info.prefetching, 64);
        break;
    case 0xF1:
        this_cpu_write(cpu_info.cache_info.prefetching, 128);
        break;
    case 0xFF:
        /*
         * Special descriptor indicating to use
         * cpuid 0x4 to determine cache information.
         */
        read_cpuid4();
        break;
    default:
        break;
    }
}

/*
 * read_cache_info:
 * Parse CPU cache and TLB information from cpuid 0x2.
 */
static int read_cache_info(void)
{
    long buf[4];
    int nreads, i;
    unsigned char *descriptor;

    nreads = 0;
    descriptor = (unsigned char *)buf;

    do {
        cpuid(2, buf[0], buf[1], buf[2], buf[3]);
        /* low byte of eax indicates number of times to call cpuid */
        if (!nreads) {
            nreads = descriptor[0];
            if (!nreads)
                return 1;
        }
        for (i = 1; i < 16; ++i)
            process_cpuid2_descriptor(descriptor[i]);
    } while (--nreads);

    set_cache_line_size();
    return 0;
}

static __always_inline unsigned long to_assoc(unsigned long n)
{
    switch (n) {
    case 2:
        return CACHE_ASSOC_2WAY;
    case 4:
        return CACHE_ASSOC_4WAY;
    case 6:
        return CACHE_ASSOC_6WAY;
    case 8:
        return CACHE_ASSOC_8WAY;
    case 12:
        return CACHE_ASSOC_12WAY;
    case 16:
        return CACHE_ASSOC_16WAY;
    case 24:
        return CACHE_ASSOC_24WAY;
    default:
        return CACHE_ASSOC_FULL;
    }
}

/*
 * read_cpuid4:
 * Extract cache information from cpuid 0x4.
 */
static void read_cpuid4(void)
{
    unsigned long buf[4];
    unsigned long cache_level, assoc, line_size, size;
    unsigned int i;

    i = 0;
    while (1) {
        /*
         * The value of register ECX tells cpuid 0x4 which cache
         * information to return.
         * This is typically 0: L1d, 1: L1i, 2: L2, 3: L3.
         * We keep trying until we run out of caches.
         */
        asm volatile("movl %0, %%ecx" : : "r"(i) : "%ecx");
        cpuid(4, buf[0], buf[1], buf[2], buf[3]);

        /* Identify the actual level of the cache. */
        cache_level = (buf[0] >> 5) & 0x7;
        if (!cache_level)
            break;

        assoc = (buf[1] >> 22) + 1;
        line_size = (buf[1] & 0xFFF) + 1;
        /*
         * cache size = ways * partitions * line size * sets
         *            = assoc * (EBX[21:12] + 1) * line_size * (ECX + 1)
         */
        size =
            assoc * (((buf[1] >> 12) & 0x3FF) + 1) * line_size * (buf[2] + 1);

        add_cache(cache_level, buf[0] & 0x1F, size, line_size, to_assoc(assoc));

        ++i;
    }
}

/* full name of processor */
static char processor_name[64];

/* TEMP */
char *cpu_name(void) { return processor_name; }

/*
 * extended_processor_info:
 * Read and extract information from the extended (0x80000000+)
 * cpuid instructions.
 */
static void extended_processor_info(void)
{
    unsigned long buf[4];
    char *pos;
    unsigned int i;

    cpuid(0x80000000, buf[0], buf[1], buf[2], buf[3]);

    /* read full processor name */
    if (buf[0] >= 0x80000004) {
        pos = processor_name;
        for (i = 0x80000002; i < 0x80000005; ++i) {
            cpuid(i, buf[0], buf[1], buf[2], buf[3]);
            memcpy(pos, buf, sizeof buf);
            pos += 0x10;
        }
    }
}

DEFINE_PER_CPU(int, processor_id);
DEFINE_PER_CPU(void *, cpu_stack);

extern unsigned char bsp_stack_top;

void bsp_init_early(void)
{
    gdt_init_early();
    idt_init_early();
    percpu_init_early();
    read_cpu_info();

    this_cpu_write(processor_id, 0);
    this_cpu_write(cpu_stack, &bsp_stack_top);
    if (cpu_supports(CPUID_PGE))
        cpu_modify_cr4(0, CR4_PGE);
}

void bsp_init(void)
{
    if (bsp_apic_init() != 0) {
        klog(KLOG_WARNING,
             "bsp_init: "
             "could not initialize APIC, falling back to 8259 PIC");
    }
    cpu_init(0);
}

int cpu_init(int ap)
{
    int err;

    if (ap) {
        if ((err = lapic_init()))
            return err;

        lapic_timer_calibrate();
    }

    pat_init();
    set_cpu_online(processor_id());

    return 0;
}

/*
 * Nothing but silly printing functions below.
 * Turn around now.
 */

static char cache_info_buf[512];

static char *assoc_str(int assoc)
{
    switch (assoc) {
    case CACHE_ASSOC_2WAY:
        return "2-way";
    case CACHE_ASSOC_4WAY:
        return "4-way";
    case CACHE_ASSOC_6WAY:
        return "6-way";
    case CACHE_ASSOC_8WAY:
        return "8-way";
    case CACHE_ASSOC_12WAY:
        return "12-way";
    case CACHE_ASSOC_16WAY:
        return "16-way";
    case CACHE_ASSOC_24WAY:
        return "24-way";
    case CACHE_ASSOC_FULL:
        return "full";
    default:
        return "";
    }
}

static char *print_tlb(char *pos)
{
    unsigned long page_size, assoc;
    int printed = 0;

    page_size = this_cpu_read(cpu_info.cache_info.tlbi_page_size);
    if (page_size) {
        pos += sprintf(pos, "TLBi:\t\t");
        if (page_size & PAGE_SIZE_4K) {
            pos += sprintf(pos, "4K");
            printed = 1;
        }
        if (page_size & PAGE_SIZE_2M) {
            if (printed)
                *pos++ = '/';
            pos += sprintf(pos, "2M");
            printed = 1;
        }
        if (page_size & PAGE_SIZE_4M) {
            if (printed)
                *pos++ = '/';
            pos += sprintf(pos, "4M");
            printed = 1;
        }
        if (page_size & PAGE_SIZE_256M) {
            if (printed)
                *pos++ = '/';
            pos += sprintf(pos, "256M");
            printed = 1;
        }
        if (page_size & PAGE_SIZE_1G) {
            if (printed)
                *pos++ = '/';
            pos += sprintf(pos, "1G");
            printed = 1;
        }
        assoc = this_cpu_read(cpu_info.cache_info.tlbi_assoc);
        pos += sprintf(pos,
                       " pages, %lu entries, %s associativity\n",
                       this_cpu_read(cpu_info.cache_info.tlbi_entries),
                       assoc_str(assoc));
    }

    printed = 0;
    page_size = this_cpu_read(cpu_info.cache_info.tlbd_page_size);
    if (page_size) {
        pos += sprintf(pos, "TLBd:\t\t");
        if (page_size & PAGE_SIZE_4K) {
            pos += sprintf(pos, "4K");
            printed = 1;
        }
        if (page_size & PAGE_SIZE_2M) {
            if (printed)
                *pos++ = '/';
            pos += sprintf(pos, "2M");
            printed = 1;
        }
        if (page_size & PAGE_SIZE_4M) {
            if (printed)
                *pos++ = '/';
            pos += sprintf(pos, "4M");
            printed = 1;
        }
        if (page_size & PAGE_SIZE_256M) {
            if (printed)
                *pos++ = '/';
            pos += sprintf(pos, "256M");
            printed = 1;
        }
        if (page_size & PAGE_SIZE_1G) {
            if (printed)
                *pos++ = '/';
            pos += sprintf(pos, "1G");
            printed = 1;
        }
        assoc = this_cpu_read(cpu_info.cache_info.tlbd_assoc);
        pos += sprintf(pos,
                       " pages, %lu entries, %s associativity\n",
                       this_cpu_read(cpu_info.cache_info.tlbd_entries),
                       assoc_str(assoc));
    }

    return pos;
}

#define assoc_char(a)                        \
    (((a) == CACHE_TYPE_DATA)          ? 'd' \
     : ((a) == CACHE_TYPE_INSTRUCTION) ? 'i' \
                                       : 'u')

static char *print_caches(char *pos)
{
    struct cpu_cache *cache;
    unsigned int i, ncaches;

    ncaches = this_cpu_read(cpu_info.cache_info.ncaches);
    for (i = 0; i < ncaches; ++i) {
        cache = this_cpu_ptr(&cpu_info.cache_info.caches[i]);
        pos += sprintf(pos,
                       "L%u%c:\t\t%lu KiB, %lu byte lines, "
                       "%s associativity\n",
                       cache->id & 0xF,
                       assoc_char(cache->id >> 4),
                       cache->size / KIB(1),
                       cache->line_size,
                       assoc_str(cache->associativity));
    }
    return pos;
}

/*
 * i386_cache_str:
 * Return a beautifully formatted string detailing CPU cache information.
 */
char *i386_cache_str(void)
{
    char *pos;
    int pf;

    if (!cache_info_buf[0]) {
        pos = cache_info_buf;
        pos += sprintf(pos, "CPU cache information:\n");
        pos = print_tlb(pos);
        pos = print_caches(pos);
        pf = this_cpu_read(cpu_info.cache_info.prefetching);
        if (pf)
            pos += sprintf(pos, "Prefetch:\t%lu bytes", pf);
        if (*--pos == '\n')
            *pos = '\0';
    }

    return cache_info_buf;
}
