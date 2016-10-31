/*
 * arch/i386/include/untitled/asm/arch_io.h
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

#ifndef ARCH_I386_UNTITLED_ASM_IO_H
#define ARCH_I386_UNTITLED_ASM_IO_H

#include <stdint.h>
#include <untitled/compiler.h>

#define __arch_outb x86_outb
#define __arch_outw x86_outw
#define __arch_outl x86_outl

#define __arch_inb x86_inb
#define __arch_inw x86_inw
#define __arch_inl x86_inl

#define __arch_io_wait x86_io_wait

static __always_inline void x86_outb(uint16_t port, uint8_t val)
{
	asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static __always_inline void x86_outw(uint16_t port, uint16_t val)
{
	asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static __always_inline void x86_outl(uint16_t port, uint32_t val)
{
	asm volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static __always_inline uint8_t x86_inb(uint16_t port)
{
	uint8_t ret;

	asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

static __always_inline uint16_t x86_inw(uint16_t port)
{
	uint16_t ret;

	asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

static __always_inline uint32_t x86_inl(uint16_t port)
{
	uint32_t ret;

	asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

static __always_inline void x86_io_wait(void)
{
	asm volatile("outb %%al, $0x80" : : "a"(0));
}

#endif /* ARCH_I386_UNTITLED_ASM_IO_H */
