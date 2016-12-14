/*
 * lib/libc/stdio/printf.h
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

#ifndef LIB_LIBC_STDIO_PRINTF_H
#define LIB_LIBC_STDIO_PRINTF_H

#define FLAGS_ZERO      0x01    /* pad with zeros */
#define FLAGS_LOWER     0x02    /* lowercase in hex */
#define FLAGS_SHORT     0x04    /* short int */
#define FLAGS_LONG      0x08    /* long int */
#define FLAGS_LLONG     0x10    /* long long int */

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
	unsigned int    type:8;
	unsigned int    base:8;
	unsigned int    flags:8;
	signed int      width:24;
	signed int      precision:16;
} __attribute__((packed));

#define va_int_type(i, ap, p, sign) \
	do { \
		if (p.flags & FLAGS_LLONG) \
			i = va_arg(ap, sign long long); \
		else if (p.flags & FLAGS_LONG) \
			i = (sign long)va_arg(ap, sign long); \
		else if (p.flags & FLAGS_SHORT)\
			i = (sign short)va_arg(ap, sign int); \
		else \
			i = (sign int)va_arg(ap, sign int); \
	} while (0)

int get_format(const char *format, struct printf_format *p);

int oct_num(char *out, unsigned long long i);
int dec_num(char *out, unsigned long long i);
int hex_num(char *out, unsigned long long i, struct printf_format *p);

#endif /* LIB_LIBC_STDIO_PRINTF_H */
