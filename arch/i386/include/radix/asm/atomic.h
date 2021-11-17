/*
 * arch/i386/include/radix/asm/atomic.h
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


#ifndef ARCH_I386_RADIX_ATOMIC_H
#define ARCH_I386_RADIX_ATOMIC_H

#ifndef RADIX_ATOMIC_H
#error only <radix/atomic.h> can be included directly
#endif

#include <radix/assert.h>
#include <radix/compiler.h>
#include <radix/kernel.h>
#include <radix/types.h>

#define __x86_atomic_op_generic(p, val, op)            \
do {                                                   \
	unsigned long __aog_irqstate;                  \
	assert(ALIGNED((uintptr_t)(p), sizeof(*(p)))); \
	irq_save(__aog_irqstate);                      \
	*((volatile typeof(p))p) op val;               \
	irq_restore(__aog_irqstate);                   \
} while (0)

#define __X86_UINTTYPE_1 uint8_t
#define __X86_UINTTYPE_2 uint16_t
#define __X86_UINTTYPE_4 uint32_t

#define __X86_SUFFIX_1 "b"
#define __X86_SUFFIX_2 "w"
#define __X86_SUFFIX_4 "l"

#define __x86_atomic_inst(p, val, inst, size)           \
	asm volatile(inst __X86_SUFFIX_##size " %1, %0" \
	             : "=m"(*p)                         \
	             : "r"(val)                         \
	             : "memory")

#define atomic_write_2(p, val) __x86_atomic_inst(p, val, "mov", 2)
#define atomic_write_4(p, val) __x86_atomic_inst(p, val, "mov", 4)
#define atomic_write_8(p, val) __x86_atomic_op_generic(p, val, =)

#define atomic_or_2(p, val) __x86_atomic_inst(p, val, "or", 2)
#define atomic_or_4(p, val) __x86_atomic_inst(p, val, "or", 4)
#define atomic_or_8(p, val) __x86_atomic_op_generic(p, val, |=)

#define atomic_and_2(p, val) __x86_atomic_inst(p, val, "and", 2)
#define atomic_and_4(p, val) __x86_atomic_inst(p, val, "and", 4)
#define atomic_and_8(p, val) __x86_atomic_op_generic(p, val, &=)

#define atomic_add_2(p, val) __x86_atomic_inst(p, val, "add", 2)
#define atomic_add_4(p, val) __x86_atomic_inst(p, val, "add", 4)
#define atomic_add_8(p, val) __x86_atomic_op_generic(p, val, +=)

#define atomic_sub_2(p, val) __x86_atomic_inst(p, val, "sub", 2)
#define atomic_sub_4(p, val) __x86_atomic_inst(p, val, "sub", 4)
#define atomic_sub_8(p, val) __x86_atomic_op_generic(p, val, -=)

#define __x86_atomic_read(p)                               \
({                                                         \
	typeof(*(p)) __ar_ret = *(volatile typeof(p))(p);  \
	__ar_ret;                                          \
})

#define atomic_read_2(p) __x86_atomic_read(p)
#define atomic_read_4(p) __x86_atomic_read(p)

#define __x86_atomic_swap(p, val, size)                   \
({                                                        \
	typeof(p) __as_p = p;                             \
	typeof(val) __as_v = val;                         \
	asm volatile("xchg" __X86_SUFFIX_##size " %0, %1" \
		     : "=r"(__as_v), "=m"(*__as_p)        \
		     : "0"(__as_v)                        \
		     : "memory");                         \
	__as_v;                                           \
})

#define atomic_swap_2(p, val) __x86_atomic_swap(p, val, 2)
#define atomic_swap_4(p, val) __x86_atomic_swap(p, val, 4)

#define __x86_atomic_cmpxchg(p, old, new, size)                    \
({                                                                 \
	typeof(*(p)) __ac_ret;                                     \
	typeof(*(p)) __ac_old = old;                               \
	typeof(*(p)) __ac_new = new;                               \
        volatile __X86_UINTTYPE_##size *__ac_p =                   \
		(volatile __X86_UINTTYPE_##size *)(p);             \
        asm volatile("lock; cmpxchg" __X86_SUFFIX_##size " %2, %1" \
                     : "=a"(__ac_ret), "+m"(*__ac_p)               \
                     : "r"(__ac_new), "0"(__ac_old)                \
                     : "memory");                                  \
        __ac_ret;                                                  \
})

#define atomic_cmpxchg_2(p, old, new) __x86_atomic_cmpxchg(p, old, new, 2)
#define atomic_cmpxchg_4(p, old, new) __x86_atomic_cmpxchg(p, old, new, 4)

extern void __read_not_implemented_for_size(void);
extern void __swap_not_implemented_for_size(void);
extern void __cmpxchg_not_implemented_for_size(void);

// TODO(frolv): Implement these.
#define atomic_read_8(p)                   \
({                                         \
	typeof(*(p)) __ar_ret = 0;         \
	__read_not_implemented_for_size(); \
	__ar_ret;                          \
})

#define atomic_swap_8(p, val)              \
({                                         \
	__swap_not_implemented_for_size(); \
	val;                               \
})

#define atomic_cmpxchg_8(p, old, new)         \
({                                            \
	__cmpxchg_not_implemented_for_size(); \
	old;                                  \
})

#endif  // ARCH_I386_RADIX_ATOMIC_H
