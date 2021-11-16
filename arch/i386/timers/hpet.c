/*
 * arch/i386/timers/hpet.c
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

#include <acpi/acpi.h>
#include <acpi/tables/hpet.h>

#include <radix/config.h>
#include <radix/klog.h>
#include <radix/mm.h>
#include <radix/time.h>
#include <radix/timer.h>
#include <radix/vmm.h>

#define HPET "HPET: "

#define HPET_REG_HPETID                 0x000
#define HPET_REG_CONFIG                 0x010
#define HPET_REG_INTERRUPT_STATUS       0x020
#define HPET_REG_COUNTER                0x0F0
#define HPET_REG_COUNTER_32_LO          0x0F0
#define HPET_REG_COUNTER_32_HI          0x0F4
#define HPET_REG_TIMER_0_CONFIG         0x100
#define HPET_REG_TIMER_0_COMPARATOR     0x108
#define HPET_REG_TIMER_0_IRQ_ROUTE      0x110
#define HPET_REG_TIMER_1_CONFIG         0x120
#define HPET_REG_TIMER_1_COMPARATOR     0x128
#define HPET_REG_TIMER_1_IRQ_ROUTE      0x130
#define HPET_REG_TIMER_2_CONFIG         0x140
#define HPET_REG_TIMER_2_COMPARATOR     0x148
#define HPET_REG_TIMER_2_IRQ_ROUTE      0x150

#define HPET_REV_ID_MASK                0xFF
#define HPET_NUM_TIM_CAP_SHIFT          8
#define HPET_NUM_TIM_CAP_MASK           0x0F
#define HPET_COUNT_SIZE_CAP             (1 << 13)
#define HPET_LEG_RT_CAP                 (1 << 15)
#define HPET_VENDOR_ID_SHIFT            16
#define HPET_COUNTER_CLK_PERIOD_SHIFT   32

#define HPET_CONFIG_ENABLE_CNF          (1 << 0)

/* HPET uses femtoseconds 10^{-15} for its period */
#define FSEC_PER_NSEC                   1000000

/*
 * The high precision event timer (HPET) is a multi-purpose x86 timer,
 * although radix uses it exclusively as a timer source. The HPET contains
 * a 32 or 64-bit counter which runs at a constant frequency >= 10 MHz,
 * making it an excellent resolution timer.
 */

static paddr_t hpet_phys;
static addr_t hpet_virt;

static int hpet_is_running = 0;

static struct timer hpet;
static uint64_t (*hpet_readfn)(void);

/* Total number of HPET ticks at last timer reset */
static uint64_t hpet_last_reset_ticks = 0;

static __always_inline uint32_t hpet_reg_read_32(int reg)
{
	return *(uint32_t *)(hpet_virt + reg);
}

static __always_inline uint64_t hpet_reg_read_64(int reg)
{
	return *(uint64_t *)(hpet_virt + reg);
}

static __always_inline void hpet_reg_write(int reg, uint64_t val)
{
	*(uint64_t *)(hpet_virt + reg) = val;
}

#if CONFIG(X86_64)
/*
 * hpet_read_64_native:
 * On systems that support atomic 64-bit reads, the HPET counter can be
 * read directly.
 */
static uint64_t hpet_read_64_native(void)
{
	return hpet_reg_read_64(HPET_REG_COUNTER);
}

#else  // CONFIG(X86_64)

/*
 * hpet_read_64_mult:
 * If a system does not support atomic 64-bit reads, the HPET counter must
 * be read using two 32-bit reads, which opens the possibility of one half
 * of the HPET counter rolling over before it can be read.
 * To account for this, the upper half of the counter is read twice, with
 * the lower half read in between, until the two upper half values match.
 */
static uint64_t hpet_read_64_mult(void)
{
	uint32_t lo, hi, h2;

	do {
		hi = hpet_reg_read_32(HPET_REG_COUNTER_32_HI);
		lo = hpet_reg_read_32(HPET_REG_COUNTER_32_LO);
		h2 = hpet_reg_read_32(HPET_REG_COUNTER_32_HI);
	} while (hi != h2);

	return ((uint64_t)hi << 32) | lo;
}
#endif  // CONFIG(X86_64)

static uint64_t hpet_read_32(void)
{
	return hpet_reg_read_32(HPET_REG_COUNTER_32_LO);
}

static uint64_t hpet_read(void)
{
	return hpet_readfn() - hpet_last_reset_ticks;
}

static uint64_t hpet_reset(void)
{
	uint64_t total_ticks, ret;

	total_ticks = hpet_readfn();
	ret = total_ticks - hpet_last_reset_ticks;
	hpet_last_reset_ticks = total_ticks;

	return ret;
}

static int hpet_enable(void)
{
	if (!hpet_is_running) {
		hpet_reg_write(HPET_REG_COUNTER, 0);
		hpet_reg_write(HPET_REG_CONFIG, HPET_CONFIG_ENABLE_CNF);
		hpet_is_running = 1;
	}

	return 0;
}

/* hpet_disable: no-op. The counter should always be kept running. */
static int hpet_disable(void)
{
	return 0;
}

static void hpet_dummy(void)
{
}

static struct timer hpet = {
	.read           = hpet_read,
	.reset          = hpet_reset,
	.start          = hpet_dummy,
	.stop           = hpet_dummy,
	.enable         = hpet_enable,
	.disable        = hpet_disable,
	.flags          = 0,
	.name           = "hpet",
	.rating         = 50,
	.timer_list     = LIST_INIT(hpet.timer_list)
};

static void hpet_init(void)
{
	uint64_t hpet_id;
	uint32_t period_ns;

	hpet_id = hpet_reg_read_64(HPET_REG_HPETID);
	period_ns = (hpet_id >> HPET_COUNTER_CLK_PERIOD_SHIFT) / FSEC_PER_NSEC;

	hpet.frequency = NSEC_PER_SEC / period_ns;
	if (hpet_id & HPET_COUNT_SIZE_CAP) {
		/* 64 bit counter */
#if CONFIG(X86_64)
		hpet_readfn = hpet_read_64_native;
#else
		hpet_readfn = hpet_read_64_mult;
#endif  // CONFIG(X86_64)
	} else {
		/* 32 bit counter */
		hpet_readfn = hpet_read_32;
		hpet.max_ticks = 0xFFFFFFFF;
	}

	klog(KLOG_INFO, HPET "period %luns (%u MHz) %s-bit",
	     period_ns, hpet.frequency / 1000000,
	     (hpet_id & HPET_COUNT_SIZE_CAP) ? "64" : "32");
}

void hpet_register(void)
{
	struct acpi_hpet *hpet_table;
	void *virt;

	hpet_table = acpi_find_table(ACPI_HPET_SIGNATURE);
	if (!hpet_table)
		return;

	virt = vmalloc(PAGE_SIZE);
	if (!virt)
		return;

	hpet_phys = hpet_table->hpet_base.address;
	hpet_virt = (addr_t)virt;
	map_page_kernel(hpet_virt, hpet_phys, PROT_WRITE, PAGE_CP_UNCACHEABLE);

	hpet_init();
	timer_register(&hpet);
}
