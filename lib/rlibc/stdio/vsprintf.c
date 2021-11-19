/*
 * lib/rlibc/stdio/vsprintf.c
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

static int write_str(char *str, const char *s, struct printf_format *p);
static int write_char(char *str, int c, struct printf_format *p);
static int write_int(char *str, long long i, struct printf_format *p);
static int write_uint(char *str, unsigned long long u, struct printf_format *p);

/*
 * vsprintf: write a formatted string to buffer str.
 *
 * Supports:
 * %c and %s,
 * %d, %u, %o, %x, %X (in regular, short (h) and long (l, ll) forms),
 * special characters (#), field width, precision, and zero-padding.
 */
int vsprintf(char *str, const char *format, va_list ap)
{
    struct printf_format p;
    unsigned long long u;
    long long i;
    int n, tmp;

    n = 0;
    while (*format) {
        if (*format != '%') {
            ++n;
            *str++ = *format++;
            continue;
        }

        format += get_format(format, &p);
        switch (p.type) {
        case FORMAT_CHAR:
            tmp = write_char(str, va_arg(ap, int), &p);
            str += tmp;
            n += tmp;
            break;
        case FORMAT_STR:
            tmp = write_str(str, va_arg(ap, const char *), &p);
            str += tmp;
            n += tmp;
            break;
        case FORMAT_INT:
            i = va_int_type(ap, p, signed);
            tmp = write_int(str, i, &p);
            str += tmp;
            n += tmp;
            break;
        case FORMAT_UINT:
            u = va_int_type(ap, p, unsigned);
            tmp = write_uint(str, u, &p);
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

static int write_str(char *str, const char *s, struct printf_format *p)
{
    int len, pad;

    len = strlen(s);
    if (p->precision && p->precision < len)
        len = p->precision;

    if ((pad = p->width - len) > 0) {
        while (pad--)
            *str++ = ' ';
    }

    strcpy(str, s);
    return max((int)p->width, len);
}

static int write_char(char *str, int c, struct printf_format *p)
{
    int pad;

    pad = p->width;
    while (pad-- > 1)
        *str++ = ' ';
    *str++ = c;

    return max((int)p->width, 1);
}

static int write_int(char *str, long long i, struct printf_format *p)
{
    int pad, len, n;
    char buf[64];

    len = 0;
    if (i < 0) {
        *str++ = '-';
        ++len;
        i = -i;
    }

    n = dec_num(p, buf, i);
    len += n;

    if ((pad = p->width - len) > 0) {
        i = (p->flags & FLAGS_ZERO) ? '0' : ' ';
        while (pad--)
            *str++ = i;
    }
    strcpy(str, buf);
    return max((int)p->width, len);
}

static int write_uint(char *str, unsigned long long u, struct printf_format *p)
{
    int pad, len, special;
    char buf[64];

    len = 0;
    special = p->flags & FLAGS_SPECIAL;

    switch (p->base) {
    case 010:
        if (special && (p->flags & FLAGS_ZERO)) {
            *str++ = '0';
            special = 0;
            ++len;
        }
        len += oct_num(p, buf, u, special);
        break;
    case 0x10:
        if (special && (p->flags & FLAGS_ZERO)) {
            *str++ = '0';
            *str++ = 'x';
            special = 0;
            len += 2;
        }
        len += hex_num(p, buf, u, special);
        break;
    default:
        len += dec_num(p, buf, u);
        break;
    }

    if ((pad = p->width - len) > 0) {
        u = (p->flags & FLAGS_ZERO) ? '0' : ' ';
        while (pad--)
            *str++ = u;
    }
    strcpy(str, buf);
    return max((int)p->width, len);
}
