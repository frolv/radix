/*
 * arch/i386/mm/pagefault.c
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

#include <radix/compiler.h>
#include <radix/cpu.h>
#include <radix/irq.h>
#include <radix/kernel.h>
#include <radix/mm.h>
#include <radix/task.h>
#include <radix/vmm.h>

#define X86_PF_PROTECTION  (1 << 0)
#define X86_PF_WRITE       (1 << 1)
#define X86_PF_USER        (1 << 2)
#define X86_PF_RESERVED    (1 << 3)
#define X86_PF_INSTRUCTION (1 << 4)

// Resolves a page fault triggered by a kernel thread.
static void do_kernel_pf(addr_t fault_addr, addr_t fault_ip, int error)
{
    struct vmm_area *area;
    struct page *p;
    const char *access;
    addr_t page;

    page = fault_addr & PAGE_MASK;
    access = error & X86_PF_WRITE ? "write to" : "read from";

    if (error & X86_PF_INSTRUCTION) {
        panic("attempt to execute from non-executable address %p [eip: %p]\n",
              fault_addr,
              fault_ip);
    }

    if (error & X86_PF_PROTECTION) {
        panic("illegal %s virtual address %p [eip: %p]\n",
              access,
              fault_addr,
              fault_ip);
    }

    area = vmm_get_allocated_area(NULL, fault_addr);
    if (!area) {
        panic("attempt to %s unallocated page %p [eip: %p]\n",
              access,
              page,
              fault_ip);
    }

    /*
     * XXX: it may be worth investigating a smarter approach to this than
     * allocating a single page at a time. For example, the number of pages
     * allocated could be proportional to the size of the virtual area,
     * with the expectation that the thread accesses more of them in the
     * near future.
     * As the system grows, time spent in page faults should be profiled to
     * determine whether optimization is necessary.
     */
    p = alloc_page(PA_USER);
    if (IS_ERR(p)) {
        /*
         * TODO: figure out the best actions to take
         * here depending on the error that occurred.
         */
        panic("do_kernel_pf: could not allocate physical page\n");
    }

    map_page_kernel(page, page_to_phys(p), PROT_WRITE, PAGE_CP_DEFAULT);
    vmm_add_area_pages(area, p);
}

void page_fault_handler(const struct interrupt_context *intctx, int error)
{
    addr_t fault_addr = cpu_read_cr2();
    addr_t fault_instruction = intctx->regs.ip;

    if (error & X86_PF_USER) {
        // TODO(frolv): Handle userspace page faults.
        panic("page fault in user task %d at address %p",
              current_task()->pid,
              fault_addr);
    } else {
        do_kernel_pf(fault_addr, fault_instruction, error);
    }
}
