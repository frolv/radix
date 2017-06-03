/*
 * include/radix/bootmsg.h
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

#ifndef RADIX_BOOTMSG_H
#define RADIX_BOOTMSG_H

#include <rlibc/stdio.h>

#define BOOT_OK_MSG(msg, ...) \
	printf("[ \x1B[1;32mOK\x1B[37m ] " msg, ##__VA_ARGS__)

#define BOOT_FAIL_MSG(msg, ...) \
	printf("[ \x1B[1;34mFAILED\x1B[37m ] " msg, ##__VA_ARGS__)

#endif /* RADIX_BOOTMSG_H */
