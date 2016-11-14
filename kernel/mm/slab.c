/*
 * kernel/mm/slab.c
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

#include <string.h>
#include <untitled/compiler.h>
#include <untitled/mm.h>
#include <untitled/page.h>
#include <untitled/slab.h>

#include "slab.h"

struct list slab_caches;

/* The cache cache caches caches. */
static struct slab_cache cache_cache;

static void __init_cache(struct slab_cache *cache, const char *name,
			 size_t size, size_t offset, int flags);

void slab_init(void)
{
	list_init(&slab_caches);

	__init_cache(&cache_cache, "cache_cache",
			sizeof (struct slab_cache), 0, 0);
	list_add(&slab_caches, &cache_cache.list);
}

/*
 * Create a new cache containing elements of size specified by size.
 * The cache is inserted into the cache list.
 */
struct slab_cache *create_cache(const char *name, size_t size, size_t offset,
				int flags)
{
	struct slab_cache *cache;

	if (!name || size < sizeof (void *) || size > KMALLOC_MAX_SIZE)
		return ERR_PTR(EINVAL);

	cache = alloc_cache(&cache_cache);

	__init_cache(cache, name, size, offset, flags);
	list_ins(&slab_caches, &cache->list);

	return cache;
}

/* Allocates a single object from the given cache. */
void *alloc_cache(struct slab_cache *cache)
{
	return NULL;
}

static void __init_cache(struct slab_cache *cache, const char *name,
			 size_t size, size_t offset, int flags)
{
	list_init(&cache->full_slabs);
	list_init(&cache->partial_slabs);
	list_init(&cache->free_slabs);

	cache->objsize = size;
	cache->flags = flags;

	cache->cache_name[0] = '\0';
	strncat(cache->cache_name, name, NAME_LEN - 1);

	list_init(&cache->list);
}
