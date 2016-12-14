/*
 * arch/i386/include/untitled/cpu.h
 * Copyright (C) 2016 Alexei Frolov
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

#ifndef ARCH_I386_UNTITLED_CPU_H
#define ARCH_I386_UNTITLED_CPU_H

#define CACHE_LINE_SIZE 64

#define EFLAGS_CF       (1 << 0)
#define EFLAGS_PF       (1 << 2)
#define EFLAGS_AF       (1 << 4)
#define EFLAGS_ZF       (1 << 6)
#define EFLAGS_SF       (1 << 7)
#define EFLAGS_TF       (1 << 8)
#define EFLAGS_IF       (1 << 9)
#define EFLAGS_DF       (1 << 10)
#define EFLAGS_OF       (1 << 11)
#define EFLAGS_IOPL     ((1 << 12) | (1 << 13))
#define EFLAGS_NT       (1 << 14)
#define EFLAGS_RF       (1 << 16)
#define EFLAGS_VM       (1 << 17)
#define EFLAGS_AC       (1 << 18)
#define EFLAGS_VIF      (1 << 19)
#define EFLAGS_VIP      (1 << 20)
#define EFLAGS_ID       (1 << 21)

#endif /* ARCH_I386_UNTITLED_CPU_H */
