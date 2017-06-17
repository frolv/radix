/*
 * include/radix/percpu_defs.h
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

#ifndef RADIX_PERCPU_DEFS_H
#define RADIX_PERCPU_DEFS_H

/*
 * Heavily inspired by Linux
 * include/linux/percpu-defs.h
 */

#include <radix/compiler.h>

#define PER_CPU_SECTION __ARCH_PER_CPU_SECTION

#define DEFINE_PER_CPU(type, name) \
	__section(PER_CPU_SECTION) typeof(type) name

#define DECLARE_PER_CPU(type, name) \
	extern DEFINE_PER_CPU(type, name)

void this_cpu_bad_size_call(void);

#define __percpu_by_size_ret(action, var)                               \
({                                                                      \
	typeof(var) __percpu_ret;                                       \
	switch (sizeof (var)) {                                         \
	case 1: __percpu_ret = this_cpu_##action##_1(var); break;       \
	case 2: __percpu_ret = this_cpu_##action##_2(var); break;       \
	case 4: __percpu_ret = this_cpu_##action##_4(var); break;       \
	case 8: __percpu_ret = this_cpu_##action##_8(var); break;       \
	default: this_cpu_bad_size_call(); break;                       \
	}                                                               \
	__percpu_ret;                                                   \
})

#define __percpu_by_size(action, var, ...)                              \
	do {                                                            \
		switch (sizeof (var)) {                                 \
		case 1: this_cpu_##action##_1(var, __VA_ARGS__); break; \
		case 2: this_cpu_##action##_2(var, __VA_ARGS__); break; \
		case 4: this_cpu_##action##_4(var, __VA_ARGS__); break; \
		case 8: this_cpu_##action##_8(var, __VA_ARGS__); break; \
		default: this_cpu_bad_size_call();                      \
		}                                                       \
	} while (0)


/* Interrupt-safe per-CPU variable operations. */
#define this_cpu_read(var)              __percpu_by_size_ret(read, var)
#define this_cpu_write(var, val)        __percpu_by_size(write, var, val)


#include <radix/mm_types.h>
#include <radix/irq.h>

/* TODO: move MAX_CPUS definition elsewhere to avoid including cpu.h here */
#include <radix/cpu.h>
extern addr_t __percpu_offset[MAX_CPUS];

/*
 * To prevent using a recursive this_cpu_read call (which the cpp doesn't like),
 * each arch will have to define its own method of getting the per-CPU offset of
 * the active processor.
 */
#define this_cpu_offset __arch_this_cpu_offset()

#define shift_percpu_ptr(ptr, off) \
	((typeof(*ptr) *)((addr_t)(ptr) + (off)))

#define raw_cpu_ptr(ptr) shift_percpu_ptr(ptr, this_cpu_offset)


#define this_cpu_ptr(ptr)                       \
({                                              \
	typeof(ptr) __tcp_ret;                  \
	irq_disable();                          \
	__tcp_ret = raw_cpu_ptr(ptr);           \
	irq_enable();                           \
	__tcp_ret;                              \
})

#define raw_cpu_read_generic(var)               \
({                                              \
	*raw_cpu_ptr(&(var));                   \
})

#define this_cpu_read_generic(var)              \
({                                              \
	typeof(var) __rg_ret;                   \
	irq_disable();                          \
	__rg_ret = raw_cpu_read_generic(var);   \
	irq_enable();                           \
	__rg_ret;                               \
})

/* TODO */
#define this_cpu_write_generic(var, val)        \
({                                              \
	(void)0;                                \
})


#ifndef this_cpu_read_1
#define this_cpu_read_1(var) this_cpu_read_generic(var)
#endif
#ifndef this_cpu_read_2
#define this_cpu_read_2(var) this_cpu_read_generic(var)
#endif
#ifndef this_cpu_read_4
#define this_cpu_read_4(var) this_cpu_read_generic(var)
#endif
#ifndef this_cpu_read_8
#define this_cpu_read_8(var) this_cpu_read_generic(var)
#endif

#ifndef this_cpu_write_1
#define this_cpu_write_1(var, val) this_cpu_write_generic(var, val)
#endif
#ifndef this_cpu_write_2
#define this_cpu_write_2(var, val) this_cpu_write_generic(var, val)
#endif
#ifndef this_cpu_write_4
#define this_cpu_write_4(var, val) this_cpu_write_generic(var, val)
#endif
#ifndef this_cpu_write_8
#define this_cpu_write_8(var, val) this_cpu_write_generic(var, val)
#endif

#endif /* RADIX_PERCPU_DEFS_H */
