/*
 * arch/i386/syscall.c
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

#include <radix/asm/idt.h>
#include <radix/asm/syscall.h>
#include <radix/asm/vectors.h>
#include <radix/syscall.h>

void arch_syscall_init(void)
{
    idt_set(VEC_SYSCALL,
            syscall,
            GDT_OFFSET(GDT_KERNEL_CODE),
            IDT_GATE_INT | IDT_DPL(3) | IDT_PRESENT);
}

void *syscall_table[X86_NUM_SYSCALLS] = {
    [X86_SYS_EXIT] = sys_exit,
};
