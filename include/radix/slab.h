/*
 * include/radix/slab.h
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

#ifndef RADIX_SLAB_H
#define RADIX_SLAB_H

#include <radix/list.h>
#include <radix/spinlock.h>

#include <stddef.h>

#define NAME_LEN 0x40

struct slab_cache {
    size_t objsize;       /* size of each cached object */
    size_t align;         /* object alignment */
    size_t offset;        /* byte offset between objects */
    size_t count;         /* number of objects per slab */
    size_t slab_ord;      /* order of pages per slab */
    unsigned long flags;  /* allocator options */
    void (*ctor)(void *); /* object constructor */
    spinlock_t lock;      /* slab spinlock */

    struct list full_slabs;    /* full slabs */
    struct list partial_slabs; /* partially full slabs */
    struct list free_slabs;    /* empty slabs */
    struct list list;          /* list of caches */

    char cache_name[NAME_LEN]; /* human-readable cache name */
};

struct slab_desc {
    struct list list;  /* list to which slab belongs */
    void *first;       /* address of first object on slab */
    size_t in_use;     /* number of objects allocated */
    unsigned int next; /* index of next object to allocate */
};

/* Slab cache creation flags */
#define SLAB_HW_CACHE_ALIGN (1 << 16) /* align objects to CPU cache */
#define SLAB_PANIC          (1 << 17) /* panic if slab creation fails */

struct slab_cache *create_cache(const char *name,
                                size_t size,
                                size_t align,
                                unsigned long flags,
                                void (*ctor)(void *));
void destroy_cache(struct slab_cache *cache);

int grow_cache(struct slab_cache *cache);
int shrink_cache(struct slab_cache *cache);

void *alloc_cache(struct slab_cache *cache);
void free_cache(struct slab_cache *cache, void *obj);

#define SLAB_MIN_ALIGN    __alignof__(unsigned long long)
#define SLAB_MIN_OBJ_SIZE (sizeof(unsigned long long))

#define KMALLOC_MAX_SIZE 0x2000

void *kmalloc(size_t size);
void kfree(void *ptr);

#endif /* RADIX_SLAB_H */
