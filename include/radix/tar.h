/*
 * include/radix/tar.h
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

#ifndef RADIX_TAR_H
#define RADIX_TAR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __USTAR_MAGIC "ustar"

#define TAR_TYPE_FILE    '0'
#define TAR_TYPE_LINK    '1'
#define TAR_TYPE_SYMLINK '2'
#define TAR_TYPE_CHARDEV '3'
#define TAR_TYPE_BLKDEV  '4'
#define TAR_TYPE_DIR     '5'
#define TAR_TYPE_FIFO    '6'

struct tar_header {
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char type;
    char link_name[100];
    char ustar[6];
    char ustar_version[2];
    char owner_name[32];
    char owner_group[32];
    char device_major[8];
    char device_minor[8];
    char prefix[155];
    char reserved[12];
    uint8_t data[];
};

static inline bool is_ustar(const struct tar_header *header)
{
    return memcmp(&header->ustar, __USTAR_MAGIC, sizeof __USTAR_MAGIC) == 0 &&
           header->ustar_version[0] == '0' && header->ustar_version[1] == '0';
}

struct tar_iter {
    const char *file_name;
    const uint8_t *file_data;
    size_t file_size;
};

// Iterates over a tar archive in memory, calling the provided function on each
// entry in the file.
void tar_foreach(const struct tar_header *header,
                 void *context,
                 void (*func)(void *context, struct tar_iter *iter));

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // RADIX_TAR_H
