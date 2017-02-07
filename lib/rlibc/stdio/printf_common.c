/*
 * lib/rlibc/stdio/printf_common.c
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

#include <rlibc/ctype.h>
#include <rlibc/string.h>

#include "printf.h"

static int atoi_skip(const char **format);

/* get_format: parse a complete single format sequence from format */
int get_format(const char *format, struct printf_format *p)
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

	if (*format == 'h') {
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

int oct_num(char *out, unsigned long long i)
{
	int len = 0;

	do {
		out[len++] = (i % 8) + '0';
	} while ((i /= 8));
	out[len] = '\0';
	strrev(out);

	return len;
}

int dec_num(char *out, unsigned long long i)
{
	int len = 0;

	do {
		out[len++] = (i % 10) + '0';
	} while ((i /= 10));
	out[len] = '\0';
	strrev(out);

	return len;
}

int hex_num(char *out, unsigned long long i, struct printf_format *p)
{
	static const char *hex_char = "ABCDEF";
	int c;
	int len = 0;

	do {
		if ((c = i % 16) < 10) {
			out[len++] = c + '0';
		} else {
			c = hex_char[c - 10];
			if (p->flags & FLAGS_LOWER)
				c = tolower(c);
			out[len++] = c;
		}
	} while ((i /= 16));
	out[len] = '\0';
	strrev(out);

	return len;
}
