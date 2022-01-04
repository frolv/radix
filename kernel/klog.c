/*
 * kernel/klog.c
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

#include <radix/compiler.h>
#include <radix/config.h>
#include <radix/console.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/smp.h>
#include <radix/spinlock.h>
#include <radix/time.h>

#include <rlibc/stdio.h>
#include <rlibc/string.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#define KLOG_MAX_MSG_LEN 256
#define KLOG_WRAPAROUND  0xFFFF

struct klog_entry {
    uint64_t timestamp;
    uint16_t msg_len;
    uint8_t level;
    uint8_t flags;
    uint32_t seqno;
    char message[];
};

static unsigned char __aligned(PAGE_SIZE)
    klog_buffer[1 << CONFIG(KLOG_SHIFT)] = {0};

static struct {
    spinlock_t lock;
    uintptr_t buffer_start;
    uintptr_t buffer_end;
    uint32_t sequence_number;
    struct klog_entry *write_cursor;

    // TODO(frolv): This shouldn't be here. It's for early debugging.
    struct console *console;
} kernel_log = {
    .lock = SPINLOCK_INIT,
    .buffer_start = (uintptr_t)klog_buffer,
    .buffer_end = (uintptr_t)klog_buffer + sizeof klog_buffer,
    .sequence_number = 0,
    .write_cursor = (struct klog_entry *)klog_buffer,
    .console = NULL,
};

static size_t klog_entry_size(const struct klog_entry *entry)
{
    return sizeof *entry + ALIGN(entry->msg_len, 8);
}

static struct klog_entry *klog_next_entry(struct klog_entry *entry)
{
    struct klog_entry *next =
        (struct klog_entry *)(((uintptr_t)entry) + klog_entry_size(entry));

    if ((uintptr_t)(next + 1) > kernel_log.buffer_end) {
        next = (struct klog_entry *)klog_buffer;
    }

    return next;
}

// Returns true if there is sufficient space in the kernel log to store a
// message of length `msg_len` at the write cursor.
static __always_inline bool klog_has_space(uint16_t msg_len)
{
    // Must always have space for an empty klog_entry struct.
    size_t required = 2 * sizeof(struct klog_entry) + ALIGN(msg_len, 8);
    return (uintptr_t)kernel_log.write_cursor + required <=
           kernel_log.buffer_end;
}

// Writes the kernel log message represented by `entry` to `buf`. The written
// string is *NOT* null-terminated. Returns the written size.
//
// `buf` should be at least (KLOG_MAX_MSG_LEN + 32) in size to guarantee the
// output fits.
static size_t klog_print(const struct klog_entry *entry, char *buf)
{
    uint32_t seconds = entry->timestamp / NSEC_PER_SEC;
    uint32_t useconds = (entry->timestamp % NSEC_PER_SEC) / NSEC_PER_USEC;

    size_t size = sprintf(buf, "[%05lu.%06lu] ", seconds, useconds);

    memcpy(buf + size, entry->message, entry->msg_len);
    buf[size + entry->msg_len] = '\n';

    return size + entry->msg_len + 1;
}

static void klog_console_write(const struct klog_entry *entry)
{
    char buf[KLOG_MAX_MSG_LEN + 32];
    size_t size = klog_print(entry, buf);
    kernel_log.console->actions->write(kernel_log.console, buf, size);
}

static void vklog(int level, const char *format, va_list ap)
{
    char msg_buf[KLOG_MAX_MSG_LEN];
    size_t msg_len = vsnprintf(msg_buf, sizeof msg_buf, format, ap);
    if (msg_len > 0 && msg_buf[msg_len - 1] == '\n') {
        --msg_len;
    }

    unsigned long irqstate;
    spin_lock_irq(&kernel_log.lock, &irqstate);

    if (!klog_has_space(msg_len)) {
        memset(kernel_log.write_cursor, 0, sizeof(*kernel_log.write_cursor));
        kernel_log.write_cursor->msg_len = KLOG_WRAPAROUND;
        kernel_log.write_cursor = (struct klog_entry *)kernel_log.buffer_start;
    }

    struct klog_entry *entry = kernel_log.write_cursor;

    entry->timestamp = time_ns();
    entry->msg_len = msg_len;
    entry->level = level;
    entry->flags = 0;
    entry->seqno = kernel_log.sequence_number++;
    memcpy(entry->message, msg_buf, msg_len);

    kernel_log.write_cursor = klog_next_entry(entry);

    spin_unlock_irq(&kernel_log.lock, irqstate);

    if (kernel_log.console && processor_id() == 0) {
        klog_console_write(entry);
    }
}

void klog(int level, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vklog(level, format, ap);
    va_end(ap);
}

void klog_set_console(struct console *c) { kernel_log.console = c; }
