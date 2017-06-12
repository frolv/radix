#
# arch/i386/cpu/early_isr.S
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

.section .text
.align 4

.macro EARLY_ISR_NOERR i
.global early_isr_\i
.type early_isr_\i, @function
early_isr_\i:
	push $0
	push $\i
	jmp early_isr_common
.endm

.macro EARLY_ISR_ERR i
.global early_isr_\i
.type early_isr_\i, @function
early_isr_\i:
	push $\i
	jmp early_isr_common
.endm

EARLY_ISR_NOERR 0
EARLY_ISR_NOERR 1
EARLY_ISR_NOERR 2
EARLY_ISR_NOERR 3
EARLY_ISR_NOERR 4
EARLY_ISR_NOERR 5
EARLY_ISR_NOERR 6
EARLY_ISR_NOERR 7
EARLY_ISR_ERR   8
EARLY_ISR_NOERR 9
EARLY_ISR_ERR   10
EARLY_ISR_ERR   11
EARLY_ISR_ERR   12
EARLY_ISR_ERR   13
EARLY_ISR_ERR   14
EARLY_ISR_NOERR 15
EARLY_ISR_NOERR 16
EARLY_ISR_ERR   17
EARLY_ISR_NOERR 18
EARLY_ISR_NOERR 19
EARLY_ISR_NOERR 20
EARLY_ISR_NOERR 21
EARLY_ISR_NOERR 22
EARLY_ISR_NOERR 23
EARLY_ISR_NOERR 24
EARLY_ISR_NOERR 25
EARLY_ISR_NOERR 26
EARLY_ISR_NOERR 27
EARLY_ISR_NOERR 28
EARLY_ISR_NOERR 29
EARLY_ISR_ERR   30
EARLY_ISR_NOERR 31

.align 16
# Might do something here in the future
early_isr_common:
	addl $8, %esp
	iret
