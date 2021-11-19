/*
 * lib/rlibc/stdio/printf.h
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

#ifndef LIB_RLIBC_STDIO_PRINTF_H
#define LIB_RLIBC_STDIO_PRINTF_H

#define FLAGS_ZERO    (1 << 0) /* pad with zeros */
#define FLAGS_LOWER   (1 << 1) /* lowercase in hex */
#define FLAGS_SHORT   (1 << 2) /* short int */
#define FLAGS_LONG    (1 << 3) /* long int */
#define FLAGS_LLONG   (1 << 4) /* long long int */
#define FLAGS_SPECIAL (1 << 5) /* add special characters */

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
    unsigned int type : 8;
    unsigned int base : 8;
    unsigned int flags : 8;
    signed int width : 24;
    signed int precision : 16;
} __attribute__((packed));

#define va_int_type(ap, p, sign)                                     \
    ({                                                               \
        (p.flags & FLAGS_LLONG)   ? va_arg(ap, sign long long)       \
        : (p.flags & FLAGS_LONG)  ? (sign long)va_arg(ap, sign long) \
        : (p.flags & FLAGS_SHORT) ? (sign short)va_arg(ap, sign int) \
                                  : (sign int)va_arg(ap, sign int);  \
    })

int get_format(const char *format, struct printf_format *p);

int oct_num(struct printf_format *p, char *out, unsigned long long i, int sp);
int dec_num(struct printf_format *p, char *out, unsigned long long i);
int hex_num(struct printf_format *p, char *out, unsigned long long i, int sp);

#endif /* LIB_RLIBC_STDIO_PRINTF_H */
