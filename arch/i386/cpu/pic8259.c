/*
 * arch/i386/cpu/pic8259.c
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

#define PIC_IRQ_COUNT   8

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

#include <radix/asm/pic.h>

#include <radix/compiler.h>
#include <radix/io.h>
#include <radix/irq.h>

/*
 * pic8259_remap:
 * Remap the master 8259 PIC to start at vector offset1
 * and the slave 8259 PIC to start at vector offset2.
 */
static void pic8259_remap(uint32_t offset1, uint32_t offset2)
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
}

/* pic8259_eoi: send an end-of-interrupt signal to the PIC chips */
static void pic8259_eoi(unsigned int vec)
{
	unsigned int irq;

	if (vec < IRQ_BASE)
		return;

	irq = vec - IRQ_BASE;
	if (irq >= PIC_IRQ_COUNT)
		outb(PIC_SLAVE_CMD, PIC_EOI);

	outb(PIC_MASTER_CMD, PIC_EOI);
}

static void __pic8259_change_bits(int port, uint8_t clear, uint8_t set)
{
	uint8_t val;

	val = inb(port);
	val &= ~clear;
	val |= set;
	outb(port, val);
}

static void pic8259_mask(unsigned int irq)
{
	if (irq < PIC_IRQ_COUNT)
		__pic8259_change_bits(PIC_MASTER_DATA, 0, 1 << irq);
	else if (irq < 2 * PIC_IRQ_COUNT)
		__pic8259_change_bits(PIC_SLAVE_DATA, 0, 1 << (irq - 8));
}

static void pic8259_unmask(unsigned int irq)
{
	if (irq < PIC_IRQ_COUNT)
		__pic8259_change_bits(PIC_MASTER_DATA, 1 << irq, 0);
	else if (irq < 2 * PIC_IRQ_COUNT)
		__pic8259_change_bits(PIC_SLAVE_DATA, 1 << (irq - 8), 0);
}

static int pic8259_send_ipi(__unused unsigned int vec,
                            __unused cpumask_t cpumask)
{
	/* no-op */
	return 0;
}

static int pic8259_send_init(void)
{
	/* no-op */
	return 0;
}

static int pic8259_send_sipi(__unused unsigned int page)
{
	/* no-op */
	return 0;
}

static struct pic pic8259 = {
	.name           = "8259PIC",
	.irq_count      = ISA_IRQ_COUNT,
	.eoi            = pic8259_eoi,
	.mask           = pic8259_mask,
	.unmask         = pic8259_unmask,
	.send_ipi       = pic8259_send_ipi,
	.send_init      = pic8259_send_init,
	.send_sipi      = pic8259_send_sipi
};

void pic8259_init(void)
{
	pic8259_remap(IRQ_BASE, IRQ_BASE + 8);
	system_pic = &pic8259;
}

/* pic8259_disable: mask all 8259 PIC interrupts */
void pic8259_disable(void)
{
	outb(PIC_SLAVE_DATA, 0xFF);
	outb(PIC_MASTER_DATA, 0xFF);
}
