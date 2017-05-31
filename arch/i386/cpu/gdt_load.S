#
# arch/i386/cpu/gdt_load.S
# Copyright (C) 2016-2017 Alexei Frolov
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

.section .data
gdtr:
	.word 0 # size
	.long 0 # base

.section .text
.align 4

.global gdt_load
.type gdt_load, @function

gdt_load:
	movl 4(%esp), %eax
	movl %eax, gdtr + 2
	movw 8(%esp), %ax
	movw %ax, gdtr
	lgdt gdtr

	jmp $0x08, $.flush_segments
# reload data segment registers
.flush_segments:
	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	ret
