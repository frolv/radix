/*
 * kernel/elf.c
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

#include <radix/bits.h>
#include <radix/compiler.h>
#include <radix/elf.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/mm.h>
#include <radix/vmm.h>

#include <rlibc/errno.h>
#include <rlibc/string.h>

#include <stdbool.h>

#define ELF "elf: "

static bool __check_elf_magic(const struct elf32_hdr *header)
{
    return header->e_ident[EI_MAG0] == ELFMAG0 &&
           header->e_ident[EI_MAG1] == ELFMAG1 &&
           header->e_ident[EI_MAG2] == ELFMAG2 &&
           header->e_ident[EI_MAG3] == ELFMAG3;
}

// Copies ELF segment data from `copy_ptr` to physical pages, then maps them in
// the provided area of an address space. The VMM area should be of the ELF
// segment's file size; `copy_size` is its memory size.
int __elf_load_segment(struct vmm_area *area,
                       addr_t copy_addr,
                       const void *copy_ptr,
                       size_t copy_size)
{
    assert(area->size >= copy_size);

    size_t size_pages = area->size / PAGE_SIZE;
    addr_t addr = area->base;

    while (size_pages > 0) {
        size_t ord = min(PA_MAX_ORDER, log2(size_pages));
        int pages = pow2(ord);
        size_t chunk_size = pages * PAGE_SIZE;

        uint8_t *ptr = vmalloc(chunk_size);
        if (!ptr) {
            return ENOMEM;
        }

        struct page *p = alloc_pages(PA_USER, ord);
        if (IS_ERR(p)) {
            vfree(ptr);
            return ERR_VAL(p);
        }

        int err = map_pages_kernel((addr_t)ptr,
                                   page_to_phys(p),
                                   pages,
                                   PROT_WRITE,
                                   PAGE_CP_UNCACHEABLE);
        if (err != 0) {
            free_pages(p);
            vfree(ptr);
            return err;
        }

        // Memory prior to the copy address is zeroed.
        if (addr < copy_addr) {
            const size_t zero_before = min(copy_addr - addr, chunk_size);
            memset(ptr, 0, zero_before);
            ptr += zero_before;
            chunk_size -= zero_before;
        }

        if (chunk_size > 0) {
            size_t to_copy = min(chunk_size, copy_size);
            if (to_copy > 0) {
                memcpy(ptr, copy_ptr, to_copy);
                copy_ptr += to_copy;
                copy_size -= to_copy;
            }

            // Zero any remaining space after the copied data.
            if (to_copy != chunk_size) {
                memset(ptr + to_copy, 0, chunk_size - to_copy);
            }
        }

        unmap_pages((addr_t)ptr, pages);
        vfree(ptr);

        err = vmm_map_pages(area, addr, p);
        if (err != 0) {
            free_pages(p);
            return err;
        }

        size_pages -= pages;
        addr += pages * PAGE_SIZE;
    }

    return 0;
}

int __elf32_load(struct vmm_space *vmm,
                 const struct elf32_hdr *header,
                 size_t len,
                 struct elf_context *context)
{
    if (header->e_type != ET_EXEC) {
        panic("elf type %u is not yet supported", header->e_type);
    }

    if (header->e_version != EV_CURRENT) {
        klog(KLOG_INFO, ELF "invalid elf version: %u", header->e_version);
        return ENOEXEC;
    }

    if (!elf_machine_is_supported(header->e_machine)) {
        klog(KLOG_INFO,
             ELF "file compiled for wrong architecture: %u",
             header->e_machine);
        return ENOEXEC;
    }

    for (int i = 0; i < header->e_phnum; ++i) {
        struct elf32_phdr *phdr =
            (struct elf32_phdr *)((uint8_t *)header + header->e_phoff +
                                  header->e_phentsize * i);

        if (phdr->p_offset + phdr->p_filesz > len) {
            return ENOEXEC;
        }

        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        unsigned long vmm_flags = 0;
        if (phdr->p_flags & PF_X) {
            vmm_flags |= VMM_EXEC;
        }
        if (phdr->p_flags & PF_W) {
            vmm_flags |= VMM_WRITE;
        }
        if (phdr->p_flags & PF_R) {
            vmm_flags |= VMM_READ;
        }

        size_t size = ALIGN(phdr->p_filesz, PAGE_SIZE);

        struct vmm_area *area =
            vmm_alloc_addr(vmm, phdr->p_vaddr, size, vmm_flags);
        if (IS_ERR(area)) {
            return ERR_VAL(area);
        }

        // TODO(frolv): This could only load the entry segment immediately and
        // others when accessed via page fault.
        int err = __elf_load_segment(area,
                                     phdr->p_vaddr,
                                     (const uint8_t *)header + phdr->p_offset,
                                     phdr->p_memsz);
        if (err != 0) {
            return err;
        }
    }

    context->entry = header->e_entry;

    return 0;
}

int __elf64_load(__unused struct vmm_space *vmm,
                 const struct elf64_hdr *header,
                 size_t len,
                 __unused struct elf_context *context)
{
    panic("64-bit ELF not supported (file %p size %u)", header, len);
}

int elf_load(struct vmm_space *vmm,
             const void *ptr,
             size_t len,
             struct elf_context *context)
{
    const struct elf32_hdr *header = ptr;

    if (!__check_elf_magic(header)) {
        klog(KLOG_ERROR, ELF "not a valid elf file: %p", ptr);
        return ENOEXEC;
    }

    if (header->e_ident[EI_OSABI] != ELFOSABI_NONE) {
        klog(KLOG_ERROR, ELF "invalid ABI: %u", header->e_ident[EI_OSABI]);
        return ENOEXEC;
    }

    switch (header->e_ident[EI_CLASS]) {
    case ELFCLASSNONE:
        klog(KLOG_ERROR, ELF "invalid elf class");
        return ENOEXEC;
    case ELFCLASS32:
        return __elf32_load(vmm, header, len, context);
    case ELFCLASS64:
        return __elf64_load(vmm, (struct elf64_hdr *)header, len, context);
    }

    klog(KLOG_ERROR, ELF "unusual elf class: %u", header->e_ident[EI_CLASS]);
    return ENOEXEC;
}
