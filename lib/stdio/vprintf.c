/*
 * lib/stdio/vprintf.c
 * Copyright (C) 2016 Alexei Frolov
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

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <untitled/tty.h>

#define FLAGS_ZERO	0x01	/* pad with zeros */
#define FLAGS_LOWER	0x02	/* lowercase in hex */
#define FLAGS_SHORT	0x04	/* short int */
#define FLAGS_LONG	0x08	/* long int */
#define FLAGS_LLONG	0x10	/* long long int */

enum format_type {
	FORMAT_NONE,
	FORMAT_INVALID,
	FORMAT_CHAR,
	FORMAT_STR,
	FORMAT_INT,
	FORMAT_UINT,
	FORMAT_PERCENT
};

struct printf_format {
	unsigned int	type:8;
	unsigned int	base:8;
	unsigned int	flags:8;
	signed int	width:24;
	signed int	precision:16;
} __attribute__((packed));

static int get_format(const char *format, struct printf_format *p);
static int print_str(va_list ap, struct printf_format *p);

/*
 * vprintf:
 * A simple version of the vprintf(3) function, supporting basic integer
 * and string format sequences. Adapted from Linux vsprintf.
 */
int vprintf(const char *format, va_list ap)
{
	int n = 0;
	struct printf_format p;

	while (*format) {
		if (*format != '%') {
			++n;
			tty_putchar(*format++);
			continue;
		}

		format += get_format(format, &p);
		switch (p.type) {
		case FORMAT_STR:
			n += print_str(ap, &p);
			break;
		case FORMAT_PERCENT:
			putchar('%');
			break;
		}
	}

	return n;
}

static int atoi_skip(const char **format);

/* get_format: parse a complete single format sequence from format */
static int get_format(const char *format, struct printf_format *p)
{
	const char *start = format;

	p->width = -1;
	p->precision = -1;
	p->base = 10;
	p->flags = 0;
	p->type = FORMAT_NONE;

	while (*++format == '0')
		p->flags |= FLAGS_ZERO;

	if (isdigit(*format))
		p->width = atoi_skip(&format);

	if (*format == '.' && isdigit(*++format)) {
		p->precision = atoi_skip(&format);
		if (p->precision < 0)
			p->precision = 0;
	}

	if (*format ==  'h') {
		p->flags |= FLAGS_SHORT;
		++format;
	} else if (*format == 'l') {
		if (*++format == 'l') {
			p->flags |= FLAGS_LLONG;
			++format;
		} else {
			p->flags |= FLAGS_LONG;
		}
	}

	switch (*format) {
	case 'c':
		p->type = FORMAT_CHAR;
		break;
	case 'd':
		p->type = FORMAT_INT;
		break;
	case 'o':
		p->type = FORMAT_INT;
		p->base = 8;
		break;
	case 's':
		p->type = FORMAT_STR;
		break;
	case 'u':
		p->type = FORMAT_UINT;
		break;
	case 'x':
		p->flags |= FLAGS_LOWER;
	case 'X':
		p->type = FORMAT_UINT;
		p->base = 16;
		break;
	case '%':
		p->type = FORMAT_PERCENT;
		break;
	default:
		p->type = FORMAT_INVALID;
		break;
	}

	return ++format - start;
}

static int atoi_skip(const char **format)
{
	int i = 0;

	do {
		i = 10 * i + (**format - '0');
		++*format;
	} while (isdigit(**format));

	return i;
}

static int print_str(va_list ap, struct printf_format *p)
{
	const char *s;
	int len, pad, n;

	s = va_arg(ap, const char *);
	len = strlen(s);
	n = len;

	if ((pad = p->width - len) > 0) {
		n += pad;
		while (pad--)
			tty_putchar(' ');
	}

	tty_write(s, len);
	return n;
}
