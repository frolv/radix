/*
 * include/radix/initrd.h
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

#ifndef RADIX_INITRD_H
#define RADIX_INITRD_H

#include <stddef.h>
#include <stdint.h>

struct initrd_file {
    const char *path;
    const uint8_t *base;
    size_t size;
};

#ifdef __cplusplus
extern "C" {
#endif

int read_initrd(const void *ptr, size_t len);

const struct initrd_file *initrd_get_file(const char *path);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // RADIX_INITRD_H
