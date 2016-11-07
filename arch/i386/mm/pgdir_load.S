#
# arch/i386/mm/pdgir_load.S
# Copyright (C) 2016 Alexei Frolov
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

.section .text
.align 4

.global pgdir_load
.type pgdir_load, @function

pgdir_load:
	# load the base address of the page directory into cr3
	movl 4(%esp), %eax
	movl %eax, %cr3

	# enable the paging bit in cr0
	movl %cr0, %ecx
	orl $0x80000000, %ecx
	movl %ecx, %cr0
	ret
