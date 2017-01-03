/*
 * arch/i386/cpu/pic.c
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

#define PIC_MASTER      0x20
#define PIC_SLAVE       0xA0
#define PIC_MASTER_CMD  PIC_MASTER
#define PIC_MASTER_DATA (PIC_MASTER + 1)
#define PIC_SLAVE_CMD   PIC_SLAVE
#define PIC_SLAVE_DATA  (PIC_SLAVE + 1)

#define PIC_EOI         0x20

#define ICW1_ICW4       0x01	/* ICW4 toggle */
#define ICW1_SINGLE     0x02	/* single (cascade) mode */
#define ICW1_INTERVAL4  0x04	/* call address interval 4 (8) */
#define ICW1_LEVEL      0x08	/* level triggered (edge) mode */
#define ICW1_INIT       0x10	/* initialization */

#define ICW4_8086       0x01	/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       0x02	/* auto EOI */
#define ICW4_BUF_SLAVE  0x08	/* slave buffered mode */
#define ICW4_BUF_MASTER 0x0C	/* master buffered mode */
#define ICW4_SFNM       0x10	/* special fully nested */

#include <untitled/asm/io.h>

/* pic_eoi: send an end-of-interrupt signal to the PIC chips */
void pic_eoi(uint32_t irq)
{
	if (irq >= 8)
		outb(PIC_SLAVE_CMD, PIC_EOI);

	outb(PIC_MASTER_CMD, PIC_EOI);
}

/*
 * pic_remap:
 * Remap the master PIC to start at vector offset1
 * and the slave PIC to start at vector offset2.
 */
void pic_remap(uint32_t offset1, uint32_t offset2)
{
	uint8_t a1, a2;

	a1 = inb(PIC_MASTER_DATA);
	a2 = inb(PIC_SLAVE_DATA);

	outb(PIC_MASTER_CMD, ICW1_INIT | ICW1_ICW4); io_wait();
	outb(PIC_SLAVE_CMD,  ICW1_INIT | ICW1_ICW4); io_wait();

	outb(PIC_MASTER_DATA, offset1); io_wait();
	outb(PIC_SLAVE_DATA,  offset2); io_wait();

	outb(PIC_MASTER_DATA, 0x4); io_wait();
	outb(PIC_SLAVE_DATA,  0x2); io_wait();

	outb(PIC_MASTER_DATA, ICW4_8086); io_wait();
	outb(PIC_SLAVE_DATA,  ICW4_8086); io_wait();

	/* restore saved masks */
	outb(PIC_MASTER_DATA, a1);
	outb(PIC_SLAVE_DATA,  a2);

	/* TEMP: disable all interrupts except keyboard */
	outb(0x21, 0xFD);
	outb(0xA1, 0xFF);
}
