/*
 * lib/rlibc/stdio/vprintf.c
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
#include <radix/tty.h>

#include <rlibc/stdio.h>
#include <rlibc/string.h>

#include "printf.h"

static int print_str(const char *s, struct printf_format *p);
static int print_char(int c, struct printf_format *p);
static int print_int(long long i, struct printf_format *p);
static int print_uint(unsigned long long u, struct printf_format *p);

/*
 * vprintf: write a formatted string to default TTY.
 *
 * Supports:
 * %c and %s,
 * %d, %u, %o, %x, %X (in regular, short (h) and long (l, ll) forms),
 * special characters (#), field width, precision, and zero-padding.
 */
int vprintf(const char *format, va_list ap)
{
    struct printf_format p;
    unsigned long long u;
    long long i;
    int n = 0;

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
            i = va_int_type(ap, p, signed);
            n += print_int(i, &p);
            break;
        case FORMAT_UINT:
            u = va_int_type(ap, p, unsigned);
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

static int print_str(const char *s, struct printf_format *p)
{
    int len, pad;

    len = strlen(s);
    if (p->precision && p->precision < len)
        len = p->precision;

    if ((pad = p->width - len) > 0) {
        while (pad--)
            tty_putchar(' ');
    }

    tty_write(s, len);
    return max((int)p->width, len);
}

static int print_char(int c, struct printf_format *p)
{
    int pad;

    pad = p->width;
    while (pad-- > 1)
        tty_putchar(' ');
    tty_putchar(c);

    return max((int)p->width, 1);
}

/* print_int: print a signed integer */
static int print_int(long long i, struct printf_format *p)
{
    int pad, len, n;
    char buf[64];

    len = 0;
    if (i < 0) {
        tty_putchar('-');
        ++len;
        i = -i;
    }

    n = dec_num(p, buf, i);
    len += n;

    if ((pad = p->width - len) > 0) {
        i = (p->flags & FLAGS_ZERO) ? '0' : ' ';
        while (pad--)
            tty_putchar(i);
    }
    tty_write(buf, n);
    return max((int)p->width, len);
}

/* print_uint: print an unsigned integer in octal, decimal or hex format */
static int print_uint(unsigned long long u, struct printf_format *p)
{
    int pad, len, buflen, special;
    char buf[64];

    len = 0;
    special = p->flags & FLAGS_SPECIAL;

    switch (p->base) {
    case 010:
        if (special && (p->flags & FLAGS_ZERO)) {
            putchar('0');
            special = 0;
            ++len;
        }
        buflen = oct_num(p, buf, u, special);
        break;
    case 0x10:
        if (special && (p->flags & FLAGS_ZERO)) {
            putchar('0');
            putchar('x');
            special = 0;
            len += 2;
        }
        buflen = hex_num(p, buf, u, special);
        break;
    default:
        buflen = dec_num(p, buf, u);
        break;
    }

    len += buflen;
    if ((pad = p->width - len) > 0) {
        u = (p->flags & FLAGS_ZERO) ? '0' : ' ';
        while (pad--)
            tty_putchar(u);
    }
    tty_write(buf, buflen);
    return max((int)p->width, len);
}
