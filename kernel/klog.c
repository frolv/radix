/*
 * kernel/klog.c
 * Copyright (C) 2017 Alexei Frolov
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
#include <radix/kernel.h>
#include <radix/mutex.h>
#include <radix/types.h>
#include <radix/tty.h>

#include <rlibc/stdio.h>
#include <rlibc/string.h>

#include <stdarg.h>

#ifdef CONFIG_KLOG_SHIFT
#define KLOG_SHIFT CONFIG_KLOG_SHIFT
#else
#define KLOG_SHIFT 19
#endif

#define KLOG_MAX_MSG_LEN 256
#define KLOG_WRAPAROUND  0xFFFF

struct klog_entry {
	uint64_t        timestamp;
	uint16_t        msg_len;
	uint8_t         level;
	uint8_t         flags;
	uint32_t        seqno;
	char            message[];
};

static unsigned char klog_buffer[1 << KLOG_SHIFT];
static uintptr_t klog_end = (uintptr_t)klog_buffer + sizeof klog_buffer;

/* TODO: change this to a spinlock when we get those */
static struct mutex klog_mutex = MUTEX_INIT(klog_mutex);

static uint32_t klog_sequence_number = 0;

static struct klog_entry *first_entry = (struct klog_entry *)klog_buffer;
static struct klog_entry *next_entry  = (struct klog_entry *)klog_buffer;

static int klog_written = 0;

#define klog_entry_size(entry) \
	(sizeof *(entry) + ALIGN((entry)->msg_len, 8))

#define klog_next_entry(entry)                                  \
({                                                              \
	typeof(entry) __kne_ret;                                \
	__kne_ret = (typeof(entry))((uintptr_t)(entry) +        \
	                            klog_entry_size(entry));    \
	if (__kne_ret->msg_len == KLOG_WRAPAROUND)              \
		__kne_ret = (typeof(entry))klog_buffer;         \
	__kne_ret;                                              \
})

/*
 * klog_has_space:
 * Return 1 if there is sufficient space in the klog_buffer to store a
 * message of length `msg_len`.
 */
static __always_inline int klog_has_space(uint16_t msg_len)
{
	int required;

	/* must always have space for an empty klog_entry struct */
	required = 2 * sizeof (struct klog_entry) + ALIGN(msg_len, 8);

	return (uintptr_t)next_entry + required <= klog_end;
}

static void klog_advance_first(uint16_t msg_len)
{
	size_t entry_size;

	if (next_entry > first_entry) {
		/*
		 * There are two cases in which next_entry > first_entry.
		 * The first occurs when the buffer has not yet filled up once.
		 * The second is when first_entry has wrapped around, but
		 * first_entry has not.
		 * Since we have already ensured that there is sufficient
		 * space in the buffer for the message, the first message
		 * cannot be overwritten in either case.
		 */
		return;
	}

	entry_size = sizeof *next_entry + ALIGN(msg_len, 8);
	while ((uintptr_t)next_entry + entry_size > (uintptr_t)first_entry)
		first_entry = klog_next_entry(first_entry);
}

static int vklog(int level, const char *format, va_list ap)
{
	char buf[KLOG_MAX_MSG_LEN];
	int len;

	len = vsnprintf(buf, sizeof buf, format, ap);
	if (buf[len - 1] == '\n')
		--len;

	mutex_lock(&klog_mutex);
	if (!klog_has_space(len)) {
		/* wraparound to the start of the buffer */
		next_entry->msg_len = KLOG_WRAPAROUND;
		next_entry = (struct klog_entry *)klog_buffer;
	}

	/* we don't want to do this the first time a log message is written */
	if (klog_written)
		klog_advance_first(len);
	klog_written = 1;

	next_entry->timestamp = 0;
	next_entry->msg_len = len;
	next_entry->level = level;
	next_entry->flags = 0;
	next_entry->seqno = klog_sequence_number++;
	memcpy(next_entry->message, buf, len);

	next_entry = klog_next_entry(next_entry);
	mutex_unlock(&klog_mutex);

	return len;
}

int klog(int level, const char *format, ...)
{
	va_list ap;
	int n;

	va_start(ap, format);
	n = vklog(level, format, ap);
	va_end(ap);

	return n;
}
