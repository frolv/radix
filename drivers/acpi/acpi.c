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

#include <radix/asm/bios.h>
#include <radix/bootmsg.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/mm.h>
#include <radix/vmm.h>

#include <acpi/acpi.h>
#include <acpi/rsdp.h>
#include <acpi/tables/sdt.h>
#include <string.h>

#define RSDP_SIG "RSD PTR "

struct rsdt {
    struct acpi_sdt_header head;
    uint32_t sdt_addr[];
};

struct xsdt {
    struct acpi_sdt_header head;
    uint64_t sdt_addr[];
};

/* Address at which ACPI page mappings begin. */
static addr_t acpi_virt_base;

static void *sdt_base = NULL;
static size_t sdt_len = 0;
static size_t sdt_size = 0;

static void rsdt_setup(addr_t rsdt_addr);
static void xsdt_setup(addr_t xsdt_addr);
static int byte_sum(void *start, void *end);

void acpi_init(void)
{
    struct acpi_rsdp *rsdp;
    struct acpi_rsdp_2 *rsdp_2;
    int checksum;

    /*
     * The RSDP is identified by the signature "RSD PTR "
     * aligned on a 16-byte boundary.
     */
    rsdp = bios_find_signature(RSDP_SIG, 8, 16);
    if (!rsdp) {
        BOOT_FAIL_MSG("Could not locate ACPI RSDT\n");
        return;
    }

    klog(KLOG_INFO, "ACPI: RSDP %p", virt_to_phys(rsdp));
    acpi_virt_base = (addr_t)vmalloc(8 * PAGE_SIZE);

    if (rsdp->revision == 2) {
        rsdp_2 = (struct acpi_rsdp_2 *)rsdp;

        checksum = byte_sum(rsdp_2, (char *)rsdp_2 + rsdp_2->length);
        if ((checksum & 0xFF) != 0)
            goto err_checksum;

        xsdt_setup((addr_t)rsdp_2->xsdt_addr);
        return;
    } else {
        checksum = byte_sum(rsdp, (char *)rsdp + sizeof *rsdp);
        if ((checksum & 0xFF) != 0)
            goto err_checksum;

        rsdt_setup(rsdp->rsdt_addr);
        return;
    }

err_checksum:
    BOOT_FAIL_MSG("invalid ACPI RSDP checksum\n");
    vfree((void *)acpi_virt_base);
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
        addr += acpi_virt_base + curr_page * PAGE_SIZE;
        ((uint32_t *)sdt_base)[i] = addr;

        if (!addr_mapped(addr)) {
            map_page_kernel(
                addr & PAGE_MASK, phys_page, PROT_WRITE, PAGE_CP_DEFAULT);
            ++curr_page;
        }

        h = (struct acpi_sdt_header *)addr;
        if (addr + h->length > ALIGN(addr, PAGE_SIZE)) {
            map_page_kernel(ALIGN(addr, PAGE_SIZE),
                            phys_page + PAGE_SIZE,
                            PROT_WRITE,
                            PAGE_CP_DEFAULT);
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
        addr += acpi_virt_base + curr_page * PAGE_SIZE;
        ((uint64_t *)sdt_base)[i] = addr;

        if (!addr_mapped(addr)) {
            map_page_kernel(
                addr & PAGE_MASK, phys_page, PROT_WRITE, PAGE_CP_DEFAULT);
            ++curr_page;
        }

#if defined(__i386__)
        h = (struct acpi_sdt_header *)(addr_t)addr;
#elif defined(__x86_64__)
        h = (struct acpi_sdt_header *)addr;
#endif
        if (addr + h->length > ALIGN(addr, PAGE_SIZE)) {
            map_page_kernel(ALIGN(addr, PAGE_SIZE),
                            phys_page + PAGE_SIZE,
                            PROT_WRITE,
                            PAGE_CP_DEFAULT);
            ++curr_page;
        }
    }
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

    klog(KLOG_INFO, "ACPI: RSDT %p", rsdt_addr);

    unmap = 0;
    rsdt = (struct rsdt *)((rsdt_addr & (PAGE_SIZE - 1)) + acpi_virt_base);

    if (!addr_mapped((addr_t)rsdt)) {
        sdt_page = rsdt_addr & PAGE_MASK;
        map_page_kernel(acpi_virt_base, sdt_page, PROT_WRITE, PAGE_CP_DEFAULT);
        unmap = 1;
    }

    checksum = byte_sum(rsdt, (char *)rsdt + rsdt->head.length);

    if ((checksum & 0xFF) != 0) {
        BOOT_FAIL_MSG("Invalid ACPI RSDT checksum\n");
        if (unmap)
            unmap_page(acpi_virt_base);
        return;
    }

    sdt_base =
        (void *)((addr_t)rsdt->sdt_addr & (PAGE_SIZE - 1)) + acpi_virt_base;
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

    klog(KLOG_INFO, "ACPI: XSDT %p", xsdt_addr);

    unmap = 0;
    xsdt = (struct xsdt *)((xsdt_addr & (PAGE_SIZE - 1)) + acpi_virt_base);

    if (!addr_mapped((addr_t)xsdt)) {
        sdt_page = xsdt_addr & PAGE_MASK;
        map_page_kernel(acpi_virt_base, sdt_page, PROT_WRITE, PAGE_CP_DEFAULT);
        unmap = 1;
    }

    checksum = byte_sum(xsdt, (char *)xsdt + xsdt->head.length);

    if ((checksum & 0xFF) != 0) {
        BOOT_FAIL_MSG("Invalid ACPI XSDT checksum\n");
        if (unmap)
            unmap_page(acpi_virt_base);
        return;
    }

    sdt_base =
        (void *)((addr_t)xsdt->sdt_addr & (PAGE_SIZE - 1)) + acpi_virt_base;
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
            h = (struct acpi_sdt_header *)(addr_t)((uint64_t *)sdt_base)[i];
#elif defined(__x86_64__)
            h = (struct acpi_sdt_header *)((uint64_t *)sdt_base)[i];
#endif
        }
        if (strncmp(h->signature, signature, 4) == 0 && acpi_valid_checksum(h))
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
