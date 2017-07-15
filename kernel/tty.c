/*
 * kernel/tty.c
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

#include <radix/console.h>
#include <radix/mutex.h>
#include <radix/tty.h>

#include <rlibc/ctype.h>
#include <rlibc/string.h>

static struct mutex flush_lock = MUTEX_INIT(flush_lock);

#define TTY_BUFSIZE 8192
#define ASCII_ESC   0x1B

static char tty_buf[TTY_BUFSIZE];
static char *tty_pos = tty_buf;

static void __tty_flush_unlocked(void);

/* tty_putchar: write character c at current tty position, and increment pos */
void tty_putchar(int c)
{
	char ch = c;

	tty_write(&ch, 1);
}

/* tty_write: write size bytes of string data to the tty */
void tty_write(const char *data, size_t size)
{
	size_t space, write;
	int flush;
	char *s;

	mutex_lock(&flush_lock);

	do {
		flush = 0;

		space = TTY_BUFSIZE - (tty_pos - tty_buf);
		if (size > space) {
			write = space;
			flush = 1;
		} else {
			write = size;
		}

		/* wrote a newline, flush buffer */
		if ((s = memchr(data, '\n', write))) {
			write = s - data + 1;
			flush = 1;
		}

		memcpy(tty_pos, data, write);
		tty_pos += write;
		data += write;
		size -= write;

		if (flush)
			__tty_flush_unlocked();
	} while (size);

	mutex_unlock(&flush_lock);
}

void tty_flush(void)
{
	mutex_lock(&flush_lock);
	__tty_flush_unlocked();
	mutex_unlock(&flush_lock);
}

static int get_ansi_command(char *s, size_t n)
{
	while (n && (isdigit(*s) || *s == ';')) {
		++s;
		--n;
	}

	return n ? *s : '\0';
}

/* set_mode: set VGA buffer colors from ANSI graphics mode */
static size_t set_mode(char *s)
{
	size_t i, n;
	int intensity;
	int fg, bg;

	n = 0;
	fg = bg = -1;
	intensity = CON_NORMAL;

	while (s[n] != 'm') {
		i = 0;

		while (isdigit(s[n])) {
			i = 10 * i + (s[n] - '0');
			++n;
		}

		if (i == 0)
			intensity = CON_NORMAL;
		else if (i == 1)
			intensity = CON_BOLD;
		else if (i >= 30 && i <= 37)
			fg = (i - 30) | intensity;
		else if (i >= 40 && i <= 47)
			bg = (i - 40) | intensity;
		else
			return 0;

		if (s[n] == ';')
			++n;
	}
	active_console->actions->set_color(active_console, fg, bg);

	return n;
}

/*
 * process_ansi_esc:
 * Process an ANSI escape sequence in string s and modify VGA buffer settings
 * accordingly. Return number of characters skipped.
 */
static size_t process_ansi_esc(char *s, size_t n)
{
	size_t ret;
	int cmd;

	if (!n)
		return 0;

	if (s[1] != '[')
		return 0;

	ret = 2;
	s += ret;
	cmd = get_ansi_command(s, n - 2);

	switch (cmd) {
	case 'J':
		if (*s != '2')
			return 0;
		active_console->actions->clear(active_console);
		++ret;
		break;
	case 'm':
		ret += set_mode(s);
		if (ret == 2)
			return 0;
		break;
	default:
		return 0;
	}

	return ret;
}

/* __tty_flush_unlocked: write tty buffer to vga text buffer */
static void __tty_flush_unlocked(void)
{
	char *s, *t;
	size_t n;

	s = tty_buf;
	while ((t = memchr(s, ASCII_ESC, tty_pos - s))) {
		active_console->actions->write(active_console, s, t - s);
		n = process_ansi_esc(t, tty_pos - t);
		if (!n)
			active_console->actions->putc(active_console, ASCII_ESC);
		else
			t += n;

		s = t + 1;
		if (s > tty_pos)
			goto pos_reset;
	}
	active_console->actions->write(active_console, s, tty_pos - s);

pos_reset:
	tty_pos = tty_buf;
}
