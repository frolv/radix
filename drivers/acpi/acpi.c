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

#include <string.h>

#include <acpi/acpi.h>
#include <acpi/rsdp.h>
#include <acpi/tables/sdt.h>

#include <untitled/kernel.h>
#include <untitled/mm.h>

#define RSDP_SIG "RSD PTR "

#define BIOS_REGION_PHYS_START  0x000E0000
#define BIOS_REGION_PHYS_END    0x00100000

#define BIOS_REGION_START       (BIOS_REGION_PHYS_START + KERNEL_VIRTUAL_BASE)
#define BIOS_REGION_END         (BIOS_REGION_PHYS_END + KERNEL_VIRTUAL_BASE)

struct rsdt {
	struct acpi_sdt_header head;
	addr_t sdt_addr[];
};

struct xsdt {
	struct acpi_sdt_header head;
	addr_t sdt_addr[];
};

static addr_t *sdt_ptr;
static size_t sdt_len;

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
 * rsdt_setup:
 * Read the RSDT descriptor to find the number
 * of APCI tables and their addresses.
 */
static void rsdt_setup(addr_t rsdt_addr)
{
	struct rsdt *rsdt;
	addr_t sdt_page;
	int checksum, unmap;

	unmap = 0;
	rsdt = (struct rsdt *)(rsdt_addr + KERNEL_VIRTUAL_BASE);

	if (!addr_mapped((addr_t)rsdt)) {
		sdt_page = rsdt_addr & PAGE_MASK;
		map_page(sdt_page + KERNEL_VIRTUAL_BASE, sdt_page);
		unmap = 1;
	}

	checksum = byte_sum(rsdt, (char *)rsdt + rsdt->head.length);

	if ((checksum & 0xFF) != 0) {
		BOOT_FAIL_MSG("Invalid ACPI checksum at address 0x%08lX\n",
		              rsdt);
		if (unmap)
			unmap_page_pgtbl(sdt_page + KERNEL_VIRTUAL_BASE);
	}

	sdt_ptr = rsdt->sdt_addr + KERNEL_VIRTUAL_BASE;
	sdt_len = (rsdt->head.length - sizeof rsdt->head) / sizeof (addr_t);
}

static void xsdt_setup(addr_t xsdt_addr)
{
	struct xsdt *xsdt;

	(void)xsdt;
	(void)xsdt_addr;
}

void *acpi_find_table(char *signature)
{
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
