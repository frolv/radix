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

#include <stdio.h>
#include <string.h>
#include <untitled/cpu.h>
#include <untitled/kernel.h>

static long cpuid_max;

/* CPU vendor string */
static char vendor_id[16];

/* cpuid 0x1 information */
static long cpu_info[4];

struct cpu_cache {
	/* id[0..3]: level; id[4..7]: type */
	unsigned char   id;
	unsigned char   associativity;
	unsigned short  line_size;
	unsigned long   size;
};

#define MAX_CACHES 10

static struct cache_info {
	struct cpu_cache        caches[MAX_CACHES];
	unsigned int            ncaches;
	unsigned long           line_size;
	unsigned long           prefetching;

	/* Instruction TLB */
	unsigned long           tlbi_page_size;
	unsigned long           tlbi_entries;
	unsigned long           tlbi_assoc;

	/* Data TLB */
	unsigned long           tlbd_page_size;
	unsigned long           tlbd_entries;
	unsigned long           tlbd_assoc;
} cache_info;

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
enum {
	CACHE_TYPE_DATA = 1,
	CACHE_TYPE_INSTRUCTION,
	CACHE_TYPE_UNIFIED
};

#define PAGE_SIZE_4K     (1 << 0)
#define PAGE_SIZE_2M     (1 << 1)
#define PAGE_SIZE_4M     (1 << 2)
#define PAGE_SIZE_256M   (1 << 3)
#define PAGE_SIZE_1G     (1 << 4)

static void add_cache(unsigned char level, unsigned char type,
                      unsigned long size, unsigned long line_size,
                      unsigned long assoc);
static void read_cache_info(void);
static void extended_processor_info(void);

void read_cpu_info(void)
{
	long vendor[4];

	if (!cpuid_supported()) {
		vendor_id[0] = '\0';
		return;
	}

	cpuid(0, cpuid_max, vendor[0], vendor[2], vendor[1]);
	vendor[3] = 0;
	memcpy(vendor_id, vendor, sizeof vendor_id);

	cpuid(1, cpu_info[0], cpu_info[1], cpu_info[2], cpu_info[3]);

	memset(&cache_info, 0, sizeof cache_info);
	if (cpuid_max >= 2) {
		read_cache_info();
	} else {
		/*
		 * If cache information cannot be read, assume
		 * default values from Intel Pentium P5.
		 */
		add_cache(1, CACHE_TYPE_DATA, _K(8), 32, CACHE_ASSOC_2WAY);
		add_cache(1, CACHE_TYPE_INSTRUCTION, _K(8), 32, CACHE_ASSOC_4WAY);
		add_cache(2, CACHE_TYPE_UNIFIED, _K(256), 32, CACHE_ASSOC_4WAY);
		cache_info.line_size = 32;

		cache_info.tlbi_page_size = PAGE_SIZE_4K;
		cache_info.tlbi_entries = 32;
		cache_info.tlbi_assoc = CACHE_ASSOC_4WAY;
		cache_info.tlbd_page_size = PAGE_SIZE_4K;
		cache_info.tlbd_entries = 64;
		cache_info.tlbd_assoc = CACHE_ASSOC_4WAY;
	}

	cpu_cache_str();
	extended_processor_info();
}

int cpu_has_apic(void)
{
	return cpu_info[3] & CPUID_APIC;
}

unsigned long cpu_cache_line_size(void)
{
	return cache_info.line_size;
}

static void add_cache(unsigned char level, unsigned char type,
                      unsigned long size, unsigned long line_size,
                      unsigned long assoc)
{
	if (cache_info.ncaches == MAX_CACHES)
		return;

	cache_info.caches[cache_info.ncaches].id = level | (type << 4);
	cache_info.caches[cache_info.ncaches].associativity = assoc;
	cache_info.caches[cache_info.ncaches].line_size = line_size;
	cache_info.caches[cache_info.ncaches].size = size;
	cache_info.ncaches++;
}

static void read_cpuid4(void);

/*
 * read_cache_info:
 * Parse CPU cache and TLB information from cpuid 0x2.
 */
static void read_cache_info(void)
{
	long buf[4];
	int nreads, i;
	unsigned char *descriptor;

	nreads = 0;
	descriptor = (unsigned char *)buf;

	do {
		cpuid(2, buf[0], buf[1], buf[2], buf[3]);
		/* low byte of eax indicate number of times to call cpuid */
		if (!nreads)
			nreads = descriptor[0];

		for (i = 1; i < 16; ++i) {
			switch (descriptor[i]) {
			case 0x00:
				/* null descriptor */
				break;
			case 0x01:
				cache_info.tlbi_page_size = PAGE_SIZE_4K;
				cache_info.tlbi_entries = 32;
				cache_info.tlbi_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0x02:
				cache_info.tlbi_page_size = PAGE_SIZE_4M;
				cache_info.tlbi_entries = 2;
				cache_info.tlbi_assoc = CACHE_ASSOC_FULL;
				break;
			case 0x03:
				cache_info.tlbd_page_size = PAGE_SIZE_4K;
				cache_info.tlbd_entries = 64;
				cache_info.tlbd_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0x04:
				cache_info.tlbd_page_size = PAGE_SIZE_4M;
				cache_info.tlbd_entries = 8;
				cache_info.tlbd_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0x05:
				cache_info.tlbd_page_size = PAGE_SIZE_4M;
				cache_info.tlbd_entries = 32;
				cache_info.tlbd_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0x06:
				add_cache(1, CACHE_TYPE_INSTRUCTION, _K(8),
				          32, CACHE_ASSOC_2WAY);
				break;
			case 0x08:
				add_cache(1, CACHE_TYPE_INSTRUCTION, _K(16),
				          32, CACHE_ASSOC_4WAY);
				break;
			case 0x09:
				add_cache(1, CACHE_TYPE_INSTRUCTION, _K(32),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0x0A:
				add_cache(1, CACHE_TYPE_DATA, _K(8),
				          32, CACHE_ASSOC_2WAY);
				break;
			case 0x0B:
				cache_info.tlbi_page_size = PAGE_SIZE_4M;
				cache_info.tlbi_entries = 4;
				cache_info.tlbi_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0x0C:
				add_cache(1, CACHE_TYPE_DATA, _K(16),
				          32, CACHE_ASSOC_4WAY);
				break;
			case 0x0D:
				add_cache(1, CACHE_TYPE_DATA, _K(16),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0x0E:
				add_cache(1, CACHE_TYPE_DATA, _K(24),
				          64, CACHE_ASSOC_6WAY);
				break;
			case 0x10:
				add_cache(1, CACHE_TYPE_DATA, _K(16),
				          32, CACHE_ASSOC_4WAY);
				break;
			case 0x15:
				add_cache(1, CACHE_TYPE_INSTRUCTION, _K(16),
				          32, CACHE_ASSOC_4WAY);
				break;
			case 0x1A:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(96),
				          64, CACHE_ASSOC_6WAY);
				break;
			case 0x1D:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(128),
				          64, CACHE_ASSOC_2WAY);
				break;
			case 0x21:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(256),
				          64, CACHE_ASSOC_8WAY);
				break;
			case 0x22:
				add_cache(3, CACHE_TYPE_UNIFIED, _K(512),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0x23:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(1),
				          64, CACHE_ASSOC_8WAY);
				break;
			case 0x24:
				add_cache(2, CACHE_TYPE_UNIFIED, _M(1),
				          64, CACHE_ASSOC_16WAY);
				break;
			case 0x25:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(2),
				          64, CACHE_ASSOC_8WAY);
				break;
			case 0x29:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(4),
				          64, CACHE_ASSOC_8WAY);
				break;
			case 0x2C:
				add_cache(1, CACHE_TYPE_DATA, _K(32),
				          64, CACHE_ASSOC_8WAY);
				break;
			case 0x30:
				add_cache(1, CACHE_TYPE_INSTRUCTION, _K(32),
				          64, CACHE_ASSOC_8WAY);
				break;
			case 0x39:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(128),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0x3A:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(192),
				          64, CACHE_ASSOC_6WAY);
				break;
			case 0x3B:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(128),
				          64, CACHE_ASSOC_2WAY);
				break;
			case 0x3C:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(256),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0x3D:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(384),
				          64, CACHE_ASSOC_6WAY);
				break;
			case 0x3E:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(512),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0x41:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(128),
				          32, CACHE_ASSOC_4WAY);
				break;
			case 0x42:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(256),
				          32, CACHE_ASSOC_4WAY);
				break;
			case 0x43:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(512),
				          32, CACHE_ASSOC_4WAY);
				break;
			case 0x44:
				add_cache(2, CACHE_TYPE_UNIFIED, _M(1),
				          32, CACHE_ASSOC_4WAY);
				break;
			case 0x45:
				add_cache(2, CACHE_TYPE_UNIFIED, _M(2),
				          32, CACHE_ASSOC_4WAY);
				break;
			case 0x46:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(4),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0x47:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(8),
				          64, CACHE_ASSOC_8WAY);
				break;
			case 0x48:
				add_cache(2, CACHE_TYPE_UNIFIED, _M(3),
				          64, CACHE_ASSOC_12WAY);
				break;
			case 0x49:
				/* TODO: L3 on P4, L2 on Core 2 */
				add_cache(3, CACHE_TYPE_UNIFIED, _M(4),
				          64, CACHE_ASSOC_16WAY);
				break;
			case 0x4A:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(6),
				          64, CACHE_ASSOC_12WAY);
				break;
			case 0x4B:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(8),
				          64, CACHE_ASSOC_16WAY);
				break;
			case 0x4C:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(12),
				          64, CACHE_ASSOC_12WAY);
				break;
			case 0x4D:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(16),
				          64, CACHE_ASSOC_16WAY);
				break;
			case 0x4E:
				add_cache(2, CACHE_TYPE_UNIFIED, _M(6),
				          64, CACHE_ASSOC_24WAY);
				break;
			case 0x4F:
				cache_info.tlbi_page_size = PAGE_SIZE_4K;
				cache_info.tlbi_entries = 32;
				cache_info.tlbi_assoc = CACHE_ASSOC_FULL;
				break;
			case 0x50:
				cache_info.tlbi_page_size = PAGE_SIZE_4K |
				                            PAGE_SIZE_2M |
				                            PAGE_SIZE_4M;
				cache_info.tlbi_entries = 64;
				cache_info.tlbi_assoc = CACHE_ASSOC_FULL;
				break;
			case 0x51:
				cache_info.tlbi_page_size = PAGE_SIZE_4K |
				                            PAGE_SIZE_2M |
				                            PAGE_SIZE_4M;
				cache_info.tlbi_entries = 128;
				cache_info.tlbi_assoc = CACHE_ASSOC_FULL;
				break;
			case 0x52:
				cache_info.tlbi_page_size = PAGE_SIZE_4K |
				                            PAGE_SIZE_2M |
				                            PAGE_SIZE_4M;
				cache_info.tlbi_entries = 256;
				cache_info.tlbi_assoc = CACHE_ASSOC_FULL;
				break;
			case 0x55:
				cache_info.tlbi_page_size = PAGE_SIZE_2M |
				                            PAGE_SIZE_4M;
				cache_info.tlbi_entries = 7;
				cache_info.tlbi_assoc = CACHE_ASSOC_FULL;
				break;
			case 0x56:
				cache_info.tlbd_page_size = PAGE_SIZE_4M;
				cache_info.tlbd_entries = 16;
				cache_info.tlbd_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0x57:
				cache_info.tlbd_page_size = PAGE_SIZE_4K;
				cache_info.tlbd_entries = 16;
				cache_info.tlbd_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0x59:
				cache_info.tlbd_page_size = PAGE_SIZE_4K;
				cache_info.tlbd_entries = 16;
				cache_info.tlbd_assoc = CACHE_ASSOC_FULL;
				break;
			case 0x5A:
				cache_info.tlbd_page_size = PAGE_SIZE_2M |
				                            PAGE_SIZE_4M;
				cache_info.tlbd_entries = 32;
				cache_info.tlbd_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0x5B:
				cache_info.tlbd_page_size = PAGE_SIZE_4K |
				                            PAGE_SIZE_4M;
				cache_info.tlbd_entries = 64;
				cache_info.tlbd_assoc = CACHE_ASSOC_FULL;
				break;
			case 0x5C:
				cache_info.tlbd_page_size = PAGE_SIZE_4K |
				                            PAGE_SIZE_4M;
				cache_info.tlbd_entries = 128;
				cache_info.tlbd_assoc = CACHE_ASSOC_FULL;
				break;
			case 0x5D:
				cache_info.tlbd_page_size = PAGE_SIZE_4K |
				                            PAGE_SIZE_4M;
				cache_info.tlbd_entries = 256;
				cache_info.tlbd_assoc = CACHE_ASSOC_FULL;
				break;
			case 0x60:
				add_cache(1, CACHE_TYPE_DATA, _K(16),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0x61:
				cache_info.tlbi_page_size = PAGE_SIZE_4K;
				cache_info.tlbi_entries = 48;
				cache_info.tlbi_assoc = CACHE_ASSOC_FULL;
				break;
			case 0x63:
				cache_info.tlbd_page_size = PAGE_SIZE_2M |
				                            PAGE_SIZE_4M;
				cache_info.tlbd_entries = 32;
				cache_info.tlbd_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0x64:
				cache_info.tlbd_page_size = PAGE_SIZE_4K;
				cache_info.tlbd_entries = 512;
				cache_info.tlbd_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0x66:
				add_cache(1, CACHE_TYPE_DATA, _K(8),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0x67:
				add_cache(1, CACHE_TYPE_DATA, _K(16),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0x68:
				add_cache(1, CACHE_TYPE_DATA, _K(32),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0x6A:
				cache_info.tlbd_page_size = PAGE_SIZE_4K;
				cache_info.tlbd_entries = 64;
				cache_info.tlbd_assoc = CACHE_ASSOC_8WAY;
				break;
			case 0x6B:
				cache_info.tlbd_page_size = PAGE_SIZE_4K;
				cache_info.tlbd_entries = 256;
				cache_info.tlbd_assoc = CACHE_ASSOC_8WAY;
				break;
			case 0x6C:
				cache_info.tlbd_page_size = PAGE_SIZE_2M |
				                            PAGE_SIZE_4M;
				cache_info.tlbd_entries = 126;
				cache_info.tlbd_assoc = CACHE_ASSOC_8WAY;
				break;
			case 0x6D:
				cache_info.tlbd_page_size = PAGE_SIZE_1G;
				cache_info.tlbd_entries = 16;
				cache_info.tlbd_assoc = CACHE_ASSOC_FULL;
				break;
			case 0x76:
				cache_info.tlbi_page_size = PAGE_SIZE_2M |
				                            PAGE_SIZE_4M;
				cache_info.tlbi_entries = 8;
				cache_info.tlbi_assoc = CACHE_ASSOC_FULL;
				break;
			case 0x77:
				add_cache(1, CACHE_TYPE_INSTRUCTION, _K(16),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0x78:
				add_cache(2, CACHE_TYPE_UNIFIED, _M(1),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0x79:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(128),
				          64, CACHE_ASSOC_8WAY);
				break;
			case 0x7A:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(256),
				          64, CACHE_ASSOC_8WAY);
				break;
			case 0x7B:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(512),
				          64, CACHE_ASSOC_8WAY);
				break;
			case 0x7C:
				add_cache(2, CACHE_TYPE_UNIFIED, _M(1),
				          64, CACHE_ASSOC_8WAY);
				break;
			case 0x7D:
				add_cache(2, CACHE_TYPE_UNIFIED, _M(2),
				          64, CACHE_ASSOC_8WAY);
				break;
			case 0x7E:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(256),
				          128, CACHE_ASSOC_8WAY);
				break;
			case 0x7F:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(512),
				          64, CACHE_ASSOC_2WAY);
				break;
			case 0x80:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(512),
				          64, CACHE_ASSOC_8WAY);
				break;
			case 0x81:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(128),
				          32, CACHE_ASSOC_8WAY);
				break;
			case 0x82:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(256),
				          32, CACHE_ASSOC_8WAY);
				break;
			case 0x83:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(512),
				          32, CACHE_ASSOC_8WAY);
				break;
			case 0x84:
				add_cache(2, CACHE_TYPE_UNIFIED, _M(1),
				          32, CACHE_ASSOC_8WAY);
				break;
			case 0x85:
				add_cache(2, CACHE_TYPE_UNIFIED, _M(2),
				          32, CACHE_ASSOC_8WAY);
				break;
			case 0x86:
				add_cache(2, CACHE_TYPE_UNIFIED, _K(512),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0x87:
				add_cache(2, CACHE_TYPE_UNIFIED, _M(1),
				          64, CACHE_ASSOC_8WAY);
				break;
			case 0x88:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(2),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0x89:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(4),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0x8A:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(8),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0x8D:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(3),
				          128, CACHE_ASSOC_12WAY);
				break;
			case 0x90:
				cache_info.tlbi_page_size = PAGE_SIZE_4K |
				                            PAGE_SIZE_256M;
				cache_info.tlbi_entries = 64;
				cache_info.tlbi_assoc = CACHE_ASSOC_FULL;
				break;
			case 0x96:
				cache_info.tlbd_page_size = PAGE_SIZE_4K |
				                            PAGE_SIZE_256M;
				cache_info.tlbd_entries = 32;
				cache_info.tlbd_assoc = CACHE_ASSOC_FULL;
				break;
			case 0xA0:
				cache_info.tlbd_page_size = PAGE_SIZE_4K;
				cache_info.tlbd_entries = 32;
				cache_info.tlbd_assoc = CACHE_ASSOC_FULL;
				break;
			case 0xB0:
				cache_info.tlbi_page_size = PAGE_SIZE_4K;
				cache_info.tlbi_entries = 128;
				cache_info.tlbi_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0xB1:
				cache_info.tlbi_page_size = PAGE_SIZE_4M;
				cache_info.tlbi_entries = 4;
				cache_info.tlbi_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0xB2:
				cache_info.tlbi_page_size = PAGE_SIZE_4K;
				cache_info.tlbi_entries = 64;
				cache_info.tlbi_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0xB3:
				cache_info.tlbd_page_size = PAGE_SIZE_4K;
				cache_info.tlbd_entries = 128;
				cache_info.tlbd_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0xB4:
				cache_info.tlbd_page_size = PAGE_SIZE_4K;
				cache_info.tlbd_entries = 256;
				cache_info.tlbd_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0xB5:
				cache_info.tlbi_page_size = PAGE_SIZE_4K;
				cache_info.tlbi_entries = 64;
				cache_info.tlbi_assoc = CACHE_ASSOC_8WAY;
				break;
			case 0xB6:
				cache_info.tlbi_page_size = PAGE_SIZE_4K;
				cache_info.tlbi_entries = 128;
				cache_info.tlbi_assoc = CACHE_ASSOC_8WAY;
				break;
			case 0xBA:
				cache_info.tlbd_page_size = PAGE_SIZE_4K;
				cache_info.tlbd_entries = 64;
				cache_info.tlbd_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0xC0:
				cache_info.tlbd_page_size = PAGE_SIZE_4K |
				                            PAGE_SIZE_4M;
				cache_info.tlbd_entries = 8;
				cache_info.tlbd_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0xC2:
				cache_info.tlbd_page_size = PAGE_SIZE_2M |
				                            PAGE_SIZE_4M;
				cache_info.tlbd_entries = 16;
				cache_info.tlbd_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0xC4:
				cache_info.tlbd_page_size = PAGE_SIZE_2M |
				                            PAGE_SIZE_4M;
				cache_info.tlbd_entries = 32;
				cache_info.tlbd_assoc = CACHE_ASSOC_4WAY;
				break;
			case 0xD0:
				add_cache(3, CACHE_TYPE_UNIFIED, _K(512),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0xD1:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(1),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0xD2:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(2),
				          64, CACHE_ASSOC_4WAY);
				break;
			case 0xD6:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(1),
				          64, CACHE_ASSOC_8WAY);
				break;
			case 0xD7:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(2),
				          64, CACHE_ASSOC_8WAY);
				break;
			case 0xD8:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(4),
				          64, CACHE_ASSOC_8WAY);
				break;
			case 0xDC:
				add_cache(3, CACHE_TYPE_UNIFIED, _K(1536),
				          64, CACHE_ASSOC_12WAY);
				break;
			case 0xDD:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(3),
				          64, CACHE_ASSOC_12WAY);
				break;
			case 0xDE:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(6),
				          64, CACHE_ASSOC_12WAY);
				break;
			case 0xE2:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(2),
				          64, CACHE_ASSOC_16WAY);
				break;
			case 0xE3:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(4),
				          64, CACHE_ASSOC_16WAY);
				break;
			case 0xE4:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(8),
				          64, CACHE_ASSOC_16WAY);
				break;
			case 0xEA:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(12),
				          64, CACHE_ASSOC_24WAY);
				break;
			case 0xEB:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(18),
				          64, CACHE_ASSOC_24WAY);
				break;
			case 0xEC:
				add_cache(3, CACHE_TYPE_UNIFIED, _M(24),
				          64, CACHE_ASSOC_24WAY);
				break;
			case 0xF0:
				cache_info.prefetching = 64;
				break;
			case 0xF1:
				cache_info.prefetching = 128;
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
	} while (--nreads);

	/* set overall line size to line size of L1 cache */
	for (i = 0; (cache_info.caches[i].id & 0xF) != 1; ++i)
		;
	cache_info.line_size = cache_info.caches[i].line_size;
}

static __always_inline unsigned long to_assoc(unsigned long n)
{
	switch(n) {
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
		asm volatile("movl %0, %%ecx;" : : "r"(i) : "%ecx");
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
		size = assoc * (((buf[1] >> 12) & 0x3FF) + 1) *
		       line_size * (buf[2] + 1);

		add_cache(cache_level, buf[0] & 0x1F, size,
		          line_size, to_assoc(assoc));

		++i;
	}
}

/* full name of processor */
static char processor_name[64];

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

/*
 * Nothing but silly printing functions below.
 * Turn around now.
 */

static char cache_info_buf[512];

static char *assoc_str(int assoc)
{
	switch(assoc) {
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
	int printed = 0;

	if (cache_info.tlbi_page_size) {
		pos += sprintf(pos, "TLBi:\t\t");
		if (cache_info.tlbi_page_size & PAGE_SIZE_4K) {
			pos += sprintf(pos, "4K");
			printed = 1;
		}
		if (cache_info.tlbi_page_size & PAGE_SIZE_2M) {
			if (printed)
				*pos++ = '/';
			pos += sprintf(pos, "2M");
			printed = 1;
		}
		if (cache_info.tlbi_page_size & PAGE_SIZE_4M) {
			if (printed)
				*pos++ = '/';
			pos += sprintf(pos, "4M");
			printed = 1;
		}
		if (cache_info.tlbi_page_size & PAGE_SIZE_256M) {
			if (printed)
				*pos++ = '/';
			pos += sprintf(pos, "256M");
			printed = 1;
		}
		if (cache_info.tlbi_page_size & PAGE_SIZE_1G) {
			if (printed)
				*pos++ = '/';
			pos += sprintf(pos, "1G");
			printed = 1;
		}
		pos += sprintf(pos, " pages, %lu entries, %s associativity\n",
			       cache_info.tlbi_entries,
			       assoc_str(cache_info.tlbi_assoc));
	}

	printed = 0;
	if (cache_info.tlbd_page_size) {
		pos += sprintf(pos, "TLBd:\t\t");
		if (cache_info.tlbd_page_size & PAGE_SIZE_4K) {
			pos += sprintf(pos, "4K");
			printed = 1;
		}
		if (cache_info.tlbd_page_size & PAGE_SIZE_2M) {
			if (printed)
				*pos++ = '/';
			pos += sprintf(pos, "2M");
			printed = 1;
		}
		if (cache_info.tlbd_page_size & PAGE_SIZE_4M) {
			if (printed)
				*pos++ = '/';
			pos += sprintf(pos, "4M");
			printed = 1;
		}
		if (cache_info.tlbd_page_size & PAGE_SIZE_256M) {
			if (printed)
				*pos++ = '/';
			pos += sprintf(pos, "256M");
			printed = 1;
		}
		if (cache_info.tlbd_page_size & PAGE_SIZE_1G) {
			if (printed)
				*pos++ = '/';
			pos += sprintf(pos, "1G");
			printed = 1;
		}
		pos += sprintf(pos, " pages, %lu entries, %s associativity\n",
			       cache_info.tlbd_entries,
			       assoc_str(cache_info.tlbd_assoc));
	}

	return pos;
}

#define assoc_char(a) \
	(((a) == CACHE_TYPE_DATA) \
		? 'd' \
		: ((a) == CACHE_TYPE_INSTRUCTION ? 'i' : 'u'))

char *print_caches(char *pos)
{
	unsigned int i;

	for (i = 0; i < cache_info.ncaches; ++i) {
		pos += sprintf(pos, "L%u%c:\t\t%lu KiB, %lu byte lines, "
		               "%s associativity\n",
		               cache_info.caches[i].id & 0xF,
		               assoc_char(cache_info.caches[i].id >> 4),
		               cache_info.caches[i].size / _K(1),
		               cache_info.caches[i].line_size,
		               assoc_str(cache_info.caches[i].associativity));
	}
	return pos;
}

/*
 * cpu_cache_str:
 * Return a beautifully formatted string detailing CPU cache information.
 */
char *cpu_cache_str(void)
{
	char *pos;

	if (!cache_info_buf[0]) {
		pos = cache_info_buf;
		pos += sprintf(pos, "CPU cache information:\n");
		pos = print_tlb(pos);
		pos = print_caches(pos);
		if (cache_info.prefetching)
			pos += sprintf(pos, "Prefetch:\t%lu bytes",
				       cache_info.prefetching);
		if (*--pos == '\n')
			*pos = '\0';
	}

	return cache_info_buf;
}
