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
static int print_str(const char *s, struct printf_format *p);
static int print_char(int c, struct printf_format *p);
static int print_int(long long i, struct printf_format *p);
static int print_uint(unsigned long long u, struct printf_format *p);

#define CHECK(val, mask) (((val) & (mask)) == (mask))

#define va_int_type(i, ap, p, sign) \
	do { \
		if (CHECK(p.flags, FLAGS_LLONG)) \
			i = va_arg(ap, sign long long); \
		else if (CHECK(p.flags, FLAGS_LONG)) \
			i = (sign long)va_arg(ap, sign long); \
		else if (CHECK(p.flags, FLAGS_SHORT))\
			i = (sign short)va_arg(ap, sign int); \
		else \
			i = (sign int)va_arg(ap, sign int); \
	} while (0)

/*
 * vprintf:
 * A simple version of the vprintf(3) function, supporting basic integer
 * and string format sequences. Adapted from Linux vsprintf.
 */
int vprintf(const char *format, va_list ap)
{
	int n = 0;
	long long i;
	unsigned long long u;
	struct printf_format p;

	while (*format) {
		if (*format != '%') {
			++n;
			tty_putchar(*format++);
			continue;
		}

		format += get_format(format, &p);
		switch (p.type) {
		case FORMAT_CHAR:
			n += print_char(va_arg(ap, int), &p);
			break;
		case FORMAT_STR:
			n += print_str(va_arg(ap, const char *), &p);
			break;
		case FORMAT_INT:
			va_int_type(i, ap, p, signed);
			n += print_int(i, &p);
			break;
		case FORMAT_UINT:
			va_int_type(u, ap, p, unsigned);
			n += print_uint(u, &p);
			break;
		case FORMAT_PERCENT:
			putchar('%');
			++n;
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
		p->type = FORMAT_UINT;
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

#define max(a, b) ((a) < (b) ? (a) : (b))

static int print_str(const char *s, struct printf_format *p)
{
	int len, pad;

	len = strlen(s);
	if ((pad = p->width - len) > 0) {
		while (pad--)
			tty_putchar(' ');
	}

	tty_write(s, len);
	return max(p->width, len);
}

static int print_char(int c, struct printf_format *p)
{
	int pad;

	pad = p->width;
	while (pad-- > 1)
		tty_putchar(' ');
	tty_putchar(c);

	return max(p->width, 1);
}

static int oct_num(char *out, unsigned long long i);
static int dec_num(char *out, unsigned long long i);
static int hex_num(char *out, unsigned long long i, struct printf_format *p);

/* print_int: print a signed integer */
static int print_int(long long i, struct printf_format *p)
{
	int pad, len, n;
	char buf[32];

	len = 0;
	if (i < 0) {
		tty_putchar('-');
		++len;
		i = -i;
	}

	n = dec_num(buf, i);
	len += n;

	if ((pad = p->width - len) > 0) {
		i = CHECK(p->flags, FLAGS_ZERO) ? '0': ' ';
		while (pad--)
			tty_putchar(i);
	}
	tty_write(buf, n);
	return max(p->width, len);
}

/* print_uint: print an unsigned integer in octal, decimal or hex format */
static int print_uint(unsigned long long u, struct printf_format *p)
{
	int pad, len;
	char buf[32];

	switch (p->base) {
	case 010:
		len = oct_num(buf, u);
		break;
	case 0x10:
		len = hex_num(buf, u, p);
		break;
	default:
		len = dec_num(buf, u);
		break;
	}

	if ((pad = p->width - len) > 0) {
		u = CHECK(p->flags, FLAGS_ZERO) ? '0': ' ';
		while (pad--)
			tty_putchar(u);
	}
	tty_write(buf, len);
	return max(p->width, len);
}

static int oct_num(char *out, unsigned long long i)
{
	int len = 0;

	do {
		out[len++] = (i % 8) + '0';
	} while ((i /= 8));
	out[len] = '\0';
	strrev(out);

	return len;
}

static int dec_num(char *out, unsigned long long i)
{
	int len = 0;

	do {
		out[len++] = (i % 10) + '0';
	} while ((i /= 10));
	out[len] = '\0';
	strrev(out);

	return len;
}

static int hex_num(char *out, unsigned long long i, struct printf_format *p)
{
	static const char *hex_char = "ABCDEF";
	int c;
	int len = 0;

	do {
		if ((c = i % 16) < 10) {
			out[len++] = c + '0';
		} else {
			c = hex_char[c - 10];
			if CHECK(p->flags, FLAGS_LOWER)
				c = tolower(c);
			out[len++] = c;
		}
	} while ((i /= 16));
	out[len] = '\0';
	strrev(out);

	return len;
}
