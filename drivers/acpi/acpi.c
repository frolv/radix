/*
 * drivers/acpi/acpi.c
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

#include <acpi/acpi.h>
#include <acpi/rsdp.h>
#include <acpi/tables/sdt.h>

#include <radix/kernel.h>
#include <radix/mm.h>

#include <rlibc/string.h>

#define RSDP_SIG "RSD PTR "

#define BIOS_REGION_PHYS_START  0x000E0000
#define BIOS_REGION_PHYS_END    0x00100000

#define BIOS_REGION_START       (BIOS_REGION_PHYS_START + KERNEL_VIRTUAL_BASE)
#define BIOS_REGION_END         (BIOS_REGION_PHYS_END + KERNEL_VIRTUAL_BASE)

struct rsdt {
	struct acpi_sdt_header head;
	uint32_t sdt_addr[];
};

struct xsdt {
	struct acpi_sdt_header head;
	uint64_t sdt_addr[];
};

static void *sdt_base;
static size_t sdt_len;
static size_t sdt_size;

static void rsdt_setup(addr_t rsdt_addr);
static void xsdt_setup(addr_t xsdt_addr);
static int byte_sum(void *start, void *end);

void acpi_init(void)
{
	struct acpi_rsdp *rsdp;
	struct acpi_rsdp_2 *rsdp_2;
	uint64_t *s;
	void *err_addr;
	int checksum;

	rsdp = NULL;
	rsdp_2 = NULL;

	/*
	 * The RSDP descriptor is located within the main BIOS memory region.
	 * It is identified by the signature "RSD PTR ", aligned on a 16-byte
	 * boundary.
	 */
	s = (void *)BIOS_REGION_START;
	for (; s < (uint64_t *)BIOS_REGION_END; s += 2) {
		if (memcmp(s, RSDP_SIG, sizeof *s) == 0) {
			rsdp = (struct acpi_rsdp *)s;
			if (rsdp->revision == 2)
				rsdp_2 = (struct acpi_rsdp_2 *)s;
			break;
		}
	}

	checksum = byte_sum(rsdp, (char *)rsdp + sizeof *rsdp);
	/* Invalid checksum. Abort. */
	if ((checksum & 0xFF) != 0) {
		err_addr = rsdp;
		goto err_checksum;
	}

	if (rsdp_2) {
		checksum = byte_sum(rsdp_2, (char *)rsdp_2 + rsdp_2->length);
		if ((checksum & 0xFF) != 0) {
			err_addr = rsdp;
			goto err_checksum;
		}

		xsdt_setup((addr_t)rsdp_2->xsdt_addr);
	} else {
		rsdt_setup(rsdp->rsdt_addr);
	}

	return;

err_checksum:
	BOOT_FAIL_MSG("Invalid ACPI checksum at address 0x%08lX\n", err_addr);
}

/*
 * convert_rsdt_addrs:
 * Convert addresses of all SDT tables referred to by RSDT to virtual addresses.
 */
static void convert_rsdt_addrs(void)
{
	struct acpi_sdt_header *h;
	unsigned int i, curr_page;
	uint32_t addr, phys_page;

	curr_page = 0;
	for (i = 0; i < sdt_len; ++i) {
		addr = ((uint32_t *)sdt_base)[i];
		phys_page = addr & PAGE_MASK;
		addr &= PAGE_SIZE - 1;
		addr += ACPI_TABLES_VIRT_BASE + curr_page * PAGE_SIZE;
		((uint32_t *)sdt_base)[i] = addr;

		if (!addr_mapped(addr)) {
			map_page(addr & PAGE_MASK, phys_page);
			++curr_page;
		}

		h = (struct acpi_sdt_header *)addr;
		if (addr + h->length > ALIGN(addr, PAGE_SIZE)) {
			map_page(ALIGN(addr, PAGE_SIZE), phys_page + PAGE_SIZE);
			++curr_page;
		}
	}
}

/*
 * convert_xsdt_addrs:
 * Convert addresses of all SDT tables referred to by XSDT to virtual addresses.
 */
static void convert_xsdt_addrs(void)
{
	struct acpi_sdt_header *h;
	unsigned int i, curr_page;
	uint64_t addr, phys_page;

	curr_page = 0;
	for (i = 0; i < sdt_len; ++i) {
		addr = ((uint64_t *)sdt_base)[i];
		phys_page = addr & PAGE_MASK;
		addr &= PAGE_SIZE - 1;
		addr += ACPI_TABLES_VIRT_BASE + curr_page * PAGE_SIZE;
		((uint64_t *)sdt_base)[i] = addr;

		if (!addr_mapped(addr)) {
			map_page(addr & PAGE_MASK, phys_page);
			++curr_page;
		}

#if defined(__i386__)
		h = (struct acpi_sdt_header *)(addr_t)addr;
#elif defined(__x86_64__)
		h = (struct acpi_sdt_header *)addr;
#endif
		if (addr + h->length > ALIGN(addr, PAGE_SIZE)) {
			map_page(ALIGN(addr, PAGE_SIZE), phys_page + PAGE_SIZE);
			++curr_page;
		}
	}
}

/*
 * rsdt_setup:
 * Read the XSDT descriptor to find the number
 * of APCI tables and their addresses.
 */
static void rsdt_setup(addr_t rsdt_addr)
{
	struct rsdt *rsdt;
	addr_t sdt_page;
	int checksum, unmap;

	unmap = 0;
	rsdt = (struct rsdt *)((rsdt_addr & (PAGE_SIZE - 1))
	                       + ACPI_TABLES_VIRT_BASE);

	if (!addr_mapped((addr_t)rsdt)) {
		sdt_page = rsdt_addr & PAGE_MASK;
		map_page(ACPI_TABLES_VIRT_BASE, sdt_page);
		unmap = 1;
	}

	checksum = byte_sum(rsdt, (char *)rsdt + rsdt->head.length);

	if ((checksum & 0xFF) != 0) {
		BOOT_FAIL_MSG("Invalid ACPI checksum at address 0x%08lX\n",
		              rsdt);
		if (unmap)
			unmap_page_pgtbl(ACPI_TABLES_VIRT_BASE);
	}

	sdt_base = (void *)((addr_t)rsdt->sdt_addr & (PAGE_SIZE - 1))
		+ ACPI_TABLES_VIRT_BASE;
	sdt_size = 4;
	sdt_len = (rsdt->head.length - sizeof rsdt->head) / sdt_size;
	convert_rsdt_addrs();
}

/*
 * xsdt_setup:
 * Read the XSDT descriptor to find the number
 * of APCI tables and their addresses.
 */
static void xsdt_setup(addr_t xsdt_addr)
{
	struct xsdt *xsdt;
	addr_t sdt_page;
	int checksum, unmap;

	unmap = 0;
	xsdt = (struct xsdt *)((xsdt_addr & (PAGE_SIZE - 1))
	                       + ACPI_TABLES_VIRT_BASE);

	if (!addr_mapped((addr_t)xsdt)) {
		sdt_page = xsdt_addr & PAGE_MASK;
		map_page(ACPI_TABLES_VIRT_BASE, sdt_page);
		unmap = 1;
	}

	checksum = byte_sum(xsdt, (char *)xsdt + xsdt->head.length);

	if ((checksum & 0xFF) != 0) {
		BOOT_FAIL_MSG("Invalid ACPI checksum at address 0x%08lX\n",
		              xsdt);
		if (unmap)
			unmap_page_pgtbl(ACPI_TABLES_VIRT_BASE);
	}

	sdt_base = (void *)((addr_t)xsdt->sdt_addr & (PAGE_SIZE - 1))
		+ ACPI_TABLES_VIRT_BASE;
	sdt_size = 8;
	sdt_len = (xsdt->head.length - sizeof xsdt->head) / sdt_size;
	convert_xsdt_addrs();
}

/*
 * apci_find_table:
 * Return a pointer to the ACPI table with given signature, it it exists.
 */
void *acpi_find_table(const char *signature)
{
	unsigned int i;
	struct acpi_sdt_header *h;

	for (i = 0; i < sdt_len; ++i) {
		if (sdt_size == 4) {
			h = (struct acpi_sdt_header *)((uint32_t *)sdt_base)[i];
		} else {
#if defined(__i386__)
			h = (struct acpi_sdt_header *)
				(addr_t)((uint64_t *)sdt_base)[i];
#elif defined(__x86_64__)
			h = (struct acpi_sdt_header *)((uint64_t *)sdt_base)[i];
#endif
		}
		if (strncmp(h->signature, signature, 4) == 0)
			return h;
	}

	return NULL;
}

static int byte_sum(void *start, void *end)
{
	int sum;
	char *s;

	sum = 0;
	for (s = start; s < (char *)end; ++s)
		sum += *s;

	return sum;
}
