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
#include <untitled/cpu.h>
#include <untitled/kernel.h>
#include <untitled/mm.h>
#include <untitled/page.h>
#include <untitled/slab.h>

#include "slab.h"

struct list slab_caches;

/* The cache cache caches caches. */
static struct slab_cache cache_cache;

static void __init_cache(struct slab_cache *cache, const char *name,
			 size_t size, size_t align, unsigned long flags);

/*
 * Slabs with objects less than this size
 * have their descriptors stored on-slab.
 */
#define ON_SLAB_LIMIT		0x200

#define SLAB_DESC_ON_SLAB	(1 << 20)

void slab_init(void)
{
	list_init(&slab_caches);

	__init_cache(&cache_cache, "cache_cache", sizeof (struct slab_cache),
			MIN_ALIGN, SLAB_HW_CACHE_ALIGN);
	list_add(&slab_caches, &cache_cache.list);

	grow_cache(&cache_cache);
	grow_cache(&cache_cache);
}

/*
 * Create a new cache containing elements of size specified by size.
 * The cache is inserted into the cache list.
 */
struct slab_cache *create_cache(const char *name, size_t size, size_t align,
				unsigned long flags)
{
	struct slab_cache *cache;

	if (!name || size < MIN_OBJ_SIZE || size > KMALLOC_MAX_SIZE)
		return ERR_PTR(EINVAL);

	cache = alloc_cache(&cache_cache);

	__init_cache(cache, name, size, align, flags);
	list_ins(&slab_caches, &cache->list);

	return cache;
}

#define FREE_OBJ_ARR(s) ((uint16_t *)(s + 1))

/* Allocates a single object from the given cache. */
void *alloc_cache(struct slab_cache *cache)
{
	int err;
	struct slab_desc *s;
	void *obj;

	if (unlikely(!cache))
		return NULL;

	if (list_empty(&cache->partial_slabs)) {
		/* grow the cache if no space exists */
		if (list_empty(&cache->free_slabs)) {
			if ((err = grow_cache(cache)))
				return ERR_PTR(err);
		}

		s = list_first_entry(&cache->free_slabs,
				struct slab_desc, list);
		list_del(&s->list);
		list_add(&cache->partial_slabs, &s->list);
	} else {
		s = list_first_entry(&cache->partial_slabs,
				struct slab_desc, list);
	}

	obj = (void *)((uintptr_t)s->first + s->next * cache->offset);
	s->next = FREE_OBJ_ARR(s)[s->next];
	s->in_use++;

	if (s->in_use == cache->count) {
		list_del(&s->list);
		list_add(&cache->full_slabs, &s->list);
	}

	return obj;
}

/* Free an object from the given cache. */
void free_cache(struct slab_cache *cache, void *obj)
{
	struct slab_desc *s;
	size_t diff, ind;

	if (unlikely(!cache || !obj))
		return;

	if (cache->flags & SLAB_DESC_ON_SLAB) {
		/* slab descriptor is at the start of the slab */
		s = (struct slab_desc *)((uintptr_t)obj & PAGE_MASK);
	} else {
		/* s = find_slab(cache, obj); */
		s = NULL;
	}

	diff = obj - s->first;
	if (unlikely(diff % cache->offset != 0)) {
		/* klog("attempt to free non-allocated address %lu\n", obj); */
		return;
	}
	ind = diff / cache->offset;

	/* update s->next to the index of the freed object */
	FREE_OBJ_ARR(s)[ind] = s->next;
	s->next = ind;

	if (s->in_use == cache->count) {
		/* slab was full; move to partial */
		list_del(&s->list);
		list_add(&cache->partial_slabs, &s->list);
	} else if (s->in_use == 1) {
		/* slab is now empty */
		list_del(&s->list);
		list_add(&cache->free_slabs, &s->list);
	}
	s->in_use--;
}

/* Allocate a new slab for the given cache. */
int grow_cache(struct slab_cache *cache)
{
	struct page *p;
	struct slab_desc *s;
	uintptr_t first;
	int i;

	if (unlikely(!cache))
		return 0;

	if (cache->flags & SLAB_DESC_ON_SLAB) {
		p = alloc_page(PA_STANDARD);
		if (IS_ERR(p))
			return ERR_VAL(p);

		s = p->mem;

		/* first object placed directly after the free object array */
		first = (uintptr_t)(s + 1) + cache->count * sizeof (uint16_t);
		s->first = (void *)ALIGN(first, cache->align);
	} else {
		s = NULL;
		/* s = kmalloc(sizeof *s + cache->count * sizeof (uint16_t)); */
		/* s->first = alloc_page(); */
	}

	list_init(&s->list);
	s->in_use = 0;
	s->next = 0;

	for (i = 0; i < cache->count; ++i)
		FREE_OBJ_ARR(s)[i] = i + 1;

	list_add(&cache->free_slabs, &s->list);
	return 0;
}

/*
 * Calcuate the alignment of objects in a slab based on a user specified
 * alignment and the object size.
 */
static size_t calculate_align(unsigned long flags, size_t align, size_t size)
{
	size_t cache_align;

	/* align objects to the CPU cache if requested */
	if (flags & SLAB_HW_CACHE_ALIGN) {
		cache_align = CACHE_LINE_SIZE;
		while (size <= cache_align / 2)
			cache_align /= 2;
		align = MAX(align, cache_align);
	}

	if (align < MIN_ALIGN)
		align = MIN_ALIGN;

	return ALIGN(align, sizeof (void *));
}

/*
 * Calculate how many object will fit on a slab with the given
 * offset between objects.
 */
static int calculate_count(size_t offset, size_t align, unsigned long flags)
{
	size_t space;
	int n;

	space = PAGE_SIZE;
	if (flags & SLAB_DESC_ON_SLAB) {
		space -= sizeof (struct slab_desc);
		/*
		 * Each object requires a uint16_t in the free object array.
		 * The first statement estimates the number of objects that
		 * will fit in the slab.
		 * However, as the objects must be aligned, the gap between the
		 * end of the array and the first object may be too large to fit
		 * the estimate.
		 */
		n = space / (offset + sizeof (uint16_t));
		if (ALIGN(n * sizeof (uint16_t), align) + n * offset > space)
			--n;
		return n;
	} else {
		return space / offset;
	}
}

static void __init_cache(struct slab_cache *cache, const char *name,
			 size_t size, size_t align, unsigned long flags)
{
	list_init(&cache->full_slabs);
	list_init(&cache->partial_slabs);
	list_init(&cache->free_slabs);

	cache->objsize = size;
	cache->align = calculate_align(flags, align, size);
	cache->offset = ALIGN(size, cache->align);
	cache->flags = flags;

	if (size < ON_SLAB_LIMIT)
		cache->flags |= SLAB_DESC_ON_SLAB;

	cache->count = calculate_count(cache->offset,
			cache->align, cache->flags);

	cache->cache_name[0] = '\0';
	strncat(cache->cache_name, name, NAME_LEN - 1);

	list_init(&cache->list);
}
