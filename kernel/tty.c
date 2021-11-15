/*
 * kernel/tty.c
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

#include <stdbool.h>

#include <radix/console.h>
#include <radix/mutex.h>
#include <radix/tty.h>

#include <rlibc/ctype.h>
#include <rlibc/string.h>

#define TTY_BUFSIZE 8192
#define ASCII_ESC   0x1B

static char tty_buf[TTY_BUFSIZE];
static char *tty_pos = tty_buf;
static struct mutex tty_mutex = MUTEX_INIT(tty_mutex);

// Returns the command character from an ANSI escape command.
static char get_ansi_command(char *s, size_t n)
{
	while (n && (isdigit(*s) || *s == ';')) {
		++s;
		--n;
	}

	return n ? *s : '\0';
}

// Set console buffer colors from an ANSI graphics mode.
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

		if (i == 0) {
			intensity = CON_NORMAL;
		} else if (i == 1) {
			intensity = CON_BOLD;
		} else if (i >= 30 && i <= 37) {
			fg = (i - 30) | intensity;
		} else if (i >= 40 && i <= 47) {
			bg = (i - 40) | intensity;
		} else {
			return 0;
		}

		if (s[n] == ';') {
			++n;
		}
	}
	active_console->actions->set_color(active_console, fg, bg);

	return n;
}

// Processes an ANSI escape sequence in string s and modifies the console
// settings accordingly. Returns the number of characters skipped.
static size_t process_ansi_esc(char *s, size_t n)
{
	if (n == 0) {
		return 0;
	}

	if (s[1] != '[') {
		return 0;
	}

	size_t ret = 2;
	s += ret;
	char cmd = get_ansi_command(s, n - ret);

	switch (cmd) {
	case 'J':
		if (*s != '2') {
			return 0;
		}
		active_console->actions->clear(active_console);
		++ret;
		break;

	case 'm': {
		int chars = set_mode(s);
		if (chars == 0) {
			return 0;
		}
		ret += chars;
		break;
	}

	default:
		return 0;
	}

	return ret;
}

static void __tty_flush_locked(void)
{
	char *pos = tty_buf;
	char *esc;

	// Split the string at ANSI escape sequences, processing each.
	while ((esc = memchr(pos, ASCII_ESC, tty_pos - pos))) {
		active_console->actions->write(active_console, pos, esc - pos);

		size_t escape_size = process_ansi_esc(esc, tty_pos - esc);
		if (escape_size == 0) {
			// If there is no escape sequence, output a literal
			// escape character.
			active_console->actions->putc(active_console, ASCII_ESC);
		}

		pos = esc + escape_size + 1;
		if (pos >= tty_pos) {
			break;
		}
	}

	// Write the remaining characters, if any.
	if (pos < tty_pos) {
		active_console->actions->write(active_console, pos,
		                               tty_pos - pos);
	}

	tty_pos = tty_buf;
}

// Writes `size` bytes of string data to the TTY.
void tty_write(const char *data, size_t size)
{
	if (!data || size == 0) {
		return;
	}

	mutex_lock(&tty_mutex);

	do {
		bool flush = false;

		size_t remaining_space = TTY_BUFSIZE - (tty_pos - tty_buf);
		size_t to_write;

		if (size > remaining_space) {
			to_write = remaining_space;
			flush = 1;
		} else {
			to_write = size;
		}

		// Split the input string at newlines, flushing after each.
		char *nl;
		if ((nl = memchr(data, '\n', to_write))) {
			to_write = nl - data + 1;
			flush = true;
		}

		memcpy(tty_pos, data, to_write);
		tty_pos += to_write;
		data += to_write;
		size -= to_write;

		if (flush) {
			__tty_flush_locked();
		}
	} while (size > 0);

	mutex_unlock(&tty_mutex);
}

// Flushes the TTY buffer to the active console.
void tty_flush(void)
{
	mutex_lock(&tty_mutex);
	__tty_flush_locked();
	mutex_unlock(&tty_mutex);
}
