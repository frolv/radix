/*
 * arch/i386/cpu/regs.c
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
#include <radix/asm/regs.h>
#include <radix/cpu.h>
#include <radix/klog.h>
#include <radix/kthread.h>
#include <radix/mm.h>
#include <radix/mm_types.h>
#include <radix/vmm.h>

// Sets up the stack and registers for a kthread to execute function func with
// argument arg.
void kthread_reg_setup(struct regs *r, addr_t stack, addr_t func, addr_t arg)
{
    uint32_t *s;

    s = (uint32_t *)stack;

    s[-1] = 0;
    s[-2] = 0;
    s[-3] = 0;
    /* argument and return address */
    s[-4] = arg;
    s[-5] = (addr_t)kthread_exit;

    r->bp = (addr_t)(s - 3);
    r->sp = (addr_t)(s - 5);
    r->ip = (addr_t)func;

    r->gs = GDT_OFFSET(GDT_GS);
    r->fs = GDT_OFFSET(GDT_FS);
    r->es = GDT_OFFSET(GDT_KERNEL_DATA);
    r->ds = GDT_OFFSET(GDT_KERNEL_DATA);
    r->ss = GDT_OFFSET(GDT_KERNEL_DATA);

    r->cs = GDT_OFFSET(GDT_KERNEL_CODE);
    r->flags = EFLAGS_IF | EFLAGS_ID;
}

extern void task_user_entry(void);

int user_task_setup(struct task *task, paddr_t stack, addr_t entry)
{
    uint8_t *stack_virt = vmalloc(PAGE_SIZE);
    if (!stack_virt) {
        return ENOMEM;
    }

    int err = map_page_kernel(
        (addr_t)stack_virt, stack, PROT_WRITE, PAGE_CP_UNCACHEABLE);
    if (err != 0) {
        vfree(stack_virt);
        return err;
    }

    addr_t ustack_user = USER_STACK_TOP;
    uint32_t *ustack_kernel = (uint32_t *)(stack_virt + PAGE_SIZE);

    // Push the task's initial context onto its user-space stack.
#define PUSH(value)                           \
    do {                                      \
        --ustack_kernel;                      \
        ustack_user -= sizeof *ustack_kernel; \
        *ustack_kernel = value;               \
    } while (0)

    // TODO(frolv): This should eventually set up argc and argv.
    PUSH(0x8badf00d);
    PUSH(0x8badf00d);
    PUSH(0x8badf00d);
    PUSH(0x8badf00d);

#undef PUSH

    unmap_pages((addr_t)stack_virt, 1);
    vfree(stack_virt);

    uint32_t initial_flags = EFLAGS_IF | EFLAGS_ID;

    // Set up a ring 3 interrupt stack frame on the task's kernel stack which
    // will execute from the task's entry point after an iret:
    //
    //   -4    ss      User stack segment
    //   -8    esp     Initial user stack pointer
    //   -12   eflags  Initial user flags
    //   -16   cs      User code segment
    //   -20   eip     User entry point
    //
    uint32_t *ks = task->stack_top;
    ks[-1] = GDT_OFFSET(GDT_USER_DATA) | 0x3;
    ks[-2] = ustack_user;
    ks[-3] = initial_flags;
    ks[-4] = GDT_OFFSET(GDT_USER_CODE) | 0x3;
    ks[-5] = entry;

    struct regs *regs = &task->regs;

    regs->sp = (addr_t)(ks - 5);
    regs->ip = (uint32_t)task_user_entry;

    regs->gs = GDT_OFFSET(GDT_GS);
    regs->fs = GDT_OFFSET(GDT_FS);
    regs->es = GDT_OFFSET(GDT_USER_DATA) | 0x3;
    regs->ds = GDT_OFFSET(GDT_USER_DATA) | 0x3;
    regs->ss = GDT_OFFSET(GDT_KERNEL_DATA);

    regs->cs = GDT_OFFSET(GDT_KERNEL_CODE);
    regs->flags = initial_flags;

    return 0;
}
