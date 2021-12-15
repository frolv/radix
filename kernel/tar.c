/*
 * kernel/tar.c
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

#include <radix/kernel.h>
#include <radix/tar.h>

// Integer values in a tar header are stored as ASCII octal strings.
static unsigned int __tar_int_value(const char *buf, size_t size)
{
    unsigned int val = 0;

    for (unsigned i = 0; i < size && buf[i] != '\0'; ++i) {
        val = val * 8 + (buf[i] - '0');
    }

    return val;
}

void tar_foreach(const struct tar_header *header,
                 void *context,
                 void (*func)(void *context, struct tar_iter *iter))
{
    while (is_ustar(header)) {
        size_t size = __tar_int_value(header->size, sizeof header->size - 1);

        struct tar_iter iter = {
            .file_name = header->filename,
            .file_data = header->data,
            .file_size = size,
        };
        func(context, &iter);

        size_t entry_size = sizeof *header + ALIGN(size, 512);
        header = (const struct tar_header *)((const char *)header + entry_size);
    }
}
