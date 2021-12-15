/*
 * kernel/initrd.c
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

#include <radix/initrd.h>
#include <radix/klog.h>
#include <radix/slab.h>
#include <radix/tar.h>

#include <rlibc/errno.h>
#include <rlibc/string.h>

#define INITRD "initrd: "

enum initrd_format {
    INITRD_TAR,
    INITRD_UNKNOWN,
};

struct initrd_context {
    const void *rd_base;
    size_t rd_size;
    struct initrd_file *files;
    size_t num_files;
    size_t capacity;
};

static void __initrd_add_file(struct initrd_context *ctx,
                              const char *filepath,
                              const uint8_t *base,
                              size_t size)
{
    if (ctx->num_files == ctx->capacity) {
        ctx->capacity *= 2;

        struct initrd_file *tmp = kmalloc(ctx->capacity * sizeof *tmp);
        memcpy(tmp, ctx->files, ctx->num_files * sizeof *ctx->files);
        kfree(ctx->files);

        ctx->files = tmp;
    }

    struct initrd_file *file = &ctx->files[ctx->num_files++];
    file->path = filepath;
    file->base = base;
    file->size = size;

    klog(KLOG_INFO, INITRD "found initrd file: %s [size %uB]", filepath, size);
}

enum initrd_format __initrd_format(const void *ptr, size_t len)
{
    if (is_ustar(ptr) && len >= sizeof(struct tar_header)) {
        return INITRD_TAR;
    }

    return INITRD_UNKNOWN;
}

static void __initrd_tar_iter_callback(void *context, struct tar_iter *iter)
{
    struct initrd_context *ctx = context;
    __initrd_add_file(ctx, iter->file_name, iter->file_data, iter->file_size);
}

static int __read_tar_initrd(struct initrd_context *ctx)
{
    tar_foreach(ctx->rd_base, ctx, __initrd_tar_iter_callback);
    return 0;
}

static struct initrd_context initrd = {};

int read_initrd(const void *ptr, size_t len)
{
    klog(KLOG_INFO, INITRD "Reading initrd");

    initrd.rd_base = ptr;
    initrd.rd_size = len;
    initrd.capacity = 4;
    initrd.num_files = 0;
    initrd.files = kmalloc(initrd.capacity * sizeof *initrd.files);

    switch (__initrd_format(ptr, len)) {
    case INITRD_TAR:
        return __read_tar_initrd(&initrd);

    case INITRD_UNKNOWN:
        klog(KLOG_ERROR, INITRD "Unknown initrd at %p size %u", ptr, len);
        return EINVAL;
    }

    return 0;
}

const struct initrd_file *initrd_get_file(const char *path)
{
    for (size_t i = 0; i < initrd.num_files; ++i) {
        if (strcmp(initrd.files[i].path, path) == 0) {
            return &initrd.files[i];
        }
    }

    return NULL;
}
