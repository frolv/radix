/*
 * lib/rlibc/stdio/vsnprintf.c
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

#include <radix/kernel.h>
#include <rlibc/stdio.h>
#include <rlibc/string.h>

#include "printf.h"

static int write_str(char *str, size_t n, const char *s,
		     struct printf_format *p);
static int write_char(char *str, size_t n, int c, struct printf_format *p);
static int write_int(char *str, size_t n, long long i, struct printf_format *p);
static int write_uint(char *str, size_t n, unsigned long long u,
		      struct printf_format *p);

/*
 * vsnprintf: write a formatted string to buffer str.
 *
 * Supports:
 * %c and %s,
 * %d, %u, %o, %x, %X (in regular, short (h) and long (l, ll) forms),
 * field width and zero-padding.
 */
int vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	struct printf_format p;
	unsigned long long u;
	long long i;
	size_t n, tmp;

	n = 0;
	while (*format && n < size - 1) {
		if (*format != '%') {
			++n;
			*str++ = *format++;
			continue;
		}

		format += get_format(format, &p);
		switch (p.type) {
		case FORMAT_CHAR:
			tmp = write_char(str, size - 1 - n,
					 va_arg(ap, int), &p);
			str += tmp;
			n += tmp;
			break;
		case FORMAT_STR:
			tmp = write_str(str, size - 1 - n,
					va_arg(ap, const char *), &p);
			str += tmp;
			n += tmp;
			break;
		case FORMAT_INT:
			i = va_int_type(ap, p, signed);
			tmp = write_int(str, size - 1 - n, i, &p);
			str += tmp;
			n += tmp;
			break;
		case FORMAT_UINT:
			u = va_int_type(ap, p, unsigned);
			tmp = write_uint(str, size - 1 - n, u, &p);
			str += tmp;
			n += tmp;
			break;
		case FORMAT_PERCENT:
			*str++ = '%';
			++n;
			break;
		}
	}
	*str = '\0';

	return n;
}

static int write_str(char *str, size_t n, const char *s, struct printf_format *p)
{
	size_t len, tmp;
	int pad;

	tmp = n;
	len = strlen(s);
	if ((pad = p->width - len) > 0) {
		while (pad-- && n) {
			*str++ = ' ';
			--n;
		}
	}

	if (!n) {
		return tmp;
	} else if (n < len) {
		strncpy(str, s, n);
		return tmp;
	} else {
		strcpy(str, s);
		return max((int)p->width, (int)len);
	}
}

static int write_char(char *str, size_t n, int c, struct printf_format *p)
{
	int pad;
	size_t tmp;

	tmp = n;
	pad = p->width;
	while (pad-- > 1 && n) {
		*str++ = ' ';
		--n;
	}

	if (n) {
		*str++ = c;
		--n;
	}

	return tmp - n;
}

static int write_int(char *str, size_t n, long long i, struct printf_format *p)
{
	size_t len, blen, tmp;
	int pad;
	char buf[32];

	if (!n)
		return 0;

	tmp = n;
	len = 0;
	if (i < 0) {
		*str++ = '-';
		++len;
		--n;
		i = -i;
	}

	blen = dec_num(buf, i);
	len += blen;

	if ((pad = p->width - len) > 0) {
		i = (p->flags & FLAGS_ZERO) ? '0': ' ';
		while (pad-- && n) {
			*str++ = i;
			--n;
		}
	}
	if (!n) {
		return tmp;
	} else if (n < blen) {
		strncpy(str, buf, n);
		return tmp;
	} else {
		strcpy(str, buf);
		return max((int)p->width, (int)len);
	}
}

static int write_uint(char *str, size_t n, unsigned long long u, struct printf_format *p)
{
	size_t len, tmp;
	int pad;
	char buf[32];

	tmp = n;
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
		u = (p->flags & FLAGS_ZERO) ? '0': ' ';
		while (pad-- && n) {
			*str++ = u;
			--n;
		}
	}

	if (!n) {
		return tmp;
	} else if (n < len) {
		strncpy(str, buf, n);
		return tmp;
	} else {
		strcpy(str, buf);
		return max((int)p->width, (int)len);
	}
}
