/*
 * kernel/mm/slab.c
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

#include <radix/compiler.h>
#include <radix/cpu.h>
#include <radix/kernel.h>
#include <radix/mm.h>
#include <radix/page.h>
#include <radix/slab.h>

#include <rlibc/stdio.h>
#include <rlibc/string.h>

#include "slab.h"

struct list slab_caches;

/* The cache cache caches caches. */
static struct slab_cache cache_cache;

static void __init_cache(struct slab_cache *cache, const char *name,
                         size_t size, size_t align, unsigned long flags,
                         void (*ctor)(void *), void (*dtor)(void *));

/*
 * Slabs with objects less than this size
 * have their descriptors stored on-slab.
 */
#define ON_SLAB_LIMIT           0x200

#define SLAB_DESC_ON_SLAB       (1 << 0)
#define SLAB_IS_GROWING         (1 << 1)

void slab_init(void)
{
	list_init(&slab_caches);

	__init_cache(&cache_cache, "cache_cache", sizeof (struct slab_cache),
	             MIN_ALIGN, SLAB_HW_CACHE_ALIGN, NULL, NULL);
	list_add(&slab_caches, &cache_cache.list);

	/* preemptively allocate space for some caches */
	grow_cache(&cache_cache);
	grow_cache(&cache_cache);

	kmalloc_init();
	BOOT_OK_MSG("Memory allocators initialized (%llu MiB total)\n",
	            totalmem / MIB(1));
}

static struct slab_desc *init_slab(struct slab_cache *cache);
static int destroy_slab(struct slab_cache *cache, struct slab_desc *s);

/*
 * Create a new cache containing elements of size specified by size.
 * The cache is inserted into the cache list.
 */
struct slab_cache *create_cache(const char *name, size_t size,
                                size_t align, unsigned long flags,
                                void (*ctor)(void *), void (*dtor)(void *))
{
	struct slab_cache *cache;

	if (unlikely(!name || size < MIN_OBJ_SIZE || size > KMALLOC_MAX_SIZE))
		return ERR_PTR(EINVAL);

	cache = alloc_cache(&cache_cache);
	if (IS_ERR(cache))
		return (void *)cache;

	__init_cache(cache, name, size, align, flags, ctor, dtor);
	list_ins(&slab_caches, &cache->list);

	return cache;
}

/* Frees all slabs from a cache and removes the cache from the system. */
void destroy_cache(struct slab_cache *cache)
{
	struct list *l, *tmp;

	list_for_each_safe(l, tmp, &cache->full_slabs) {
		destroy_slab(cache, list_entry(l, struct slab_desc, list));
		list_del(l);
	}
	list_for_each_safe(l, tmp, &cache->partial_slabs) {
		destroy_slab(cache, list_entry(l, struct slab_desc, list));
		list_del(l);
	}
	list_for_each_safe(l, tmp, &cache->free_slabs) {
		destroy_slab(cache, list_entry(l, struct slab_desc, list));
		list_del(l);
	}

	list_del(&cache->list);
	free_cache(&cache_cache, cache);
}

#define FREE_OBJ_ARR(s) ((uint16_t *)(s + 1))

/* Allocates a single object from the given cache. */
void *alloc_cache(struct slab_cache *cache)
{
	struct slab_desc *s;
	void *obj;
	int err;

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

	/* find first free object at the index of s->next and update s->next */
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
	long diff, ind;

	if (unlikely(!cache || !obj))
		return;

	s = virt_to_page(obj)->slab_desc;
	if (unlikely((uintptr_t)s == PAGE_UNINIT_MAGIC)) {
		/* klog("attempt to free non-allocated address %lu\n", obj); */
		return;
	}

	diff = obj - s->first;
	if (unlikely(!ALIGNED(diff, cache->offset) || diff < 0))
		return;
	ind = diff / cache->offset;

	if (cache->dtor)
		cache->dtor(obj);

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
	struct slab_desc *s;

	if (unlikely(!cache))
		return 0;

	s = init_slab(cache);
	if (IS_ERR(s))
		return ERR_VAL(s);
	/*
	 * Mark the cache as growing to prevent the new slabs
	 * from being deallocated before they are used.
	 */
	cache->flags |= SLAB_IS_GROWING;

	list_add(&cache->free_slabs, &s->list);
	return 0;
}

/*
 * Remove (and deallocate) all free slabs from the given cache.
 * Return the number of pages freed.
 */
int shrink_cache(struct slab_cache *cache)
{
	struct list *l, *tmp;
	int n;

	n = 0;
	list_for_each_safe(l, tmp, &cache->free_slabs) {
		n += destroy_slab(cache, list_entry(l, struct slab_desc, list));
		list_del(l);
	}

	return n;
}

/* Initialize a new slab and its objects from the given cache. */
static struct slab_desc *init_slab(struct slab_cache *cache)
{
	struct slab_desc *s;
	struct page *p;
	uintptr_t first;
	size_t i;

	if (cache->flags & SLAB_DESC_ON_SLAB) {
		p = alloc_page(PA_STANDARD);
		if (IS_ERR(p))
			return (void *)p;

		s = p->mem;

		/* first object placed directly after the free object array */
		first = (uintptr_t)(s + 1) + cache->count * sizeof (uint16_t);
		s->first = (void *)ALIGN(first, cache->offset);
	} else {
		p = alloc_pages(PA_STANDARD, cache->slab_ord);
		if (IS_ERR(p))
			return (void *)p;
		s = kmalloc(sizeof *s + cache->count * sizeof (uint16_t));
		s->first = p->mem;
	}

	list_init(&s->list);
	s->in_use = 0;
	s->next = 0;

	for (i = 0; i < cache->count; ++i)
		FREE_OBJ_ARR(s)[i] = i + 1;

	/* initiliaze all cached objects */
	if (cache->ctor) {
		for (i = 0; i < cache->count; ++i)
			cache->ctor(s->first + i * cache->offset);
	}

	p->slab_cache = cache;
	p->slab_desc = s;

	return s;
}

/* Destroy all objects on a slab and deallocate all used pages. */
static int destroy_slab(struct slab_cache *cache, struct slab_desc *s)
{
	struct page *p;
	size_t i;
	int n;

	if (cache->flags & SLAB_DESC_ON_SLAB) {
		p = (struct page *)s;
		n = 1;
	} else {
		p = s->first;
		n = pow2(PM_PAGE_BLOCK_ORDER(p));
		kfree(s);
	}

	/* destroy all cached objects */
	if (cache->dtor) {
		for (i = 0; i < cache->count; ++i)
			cache->dtor(s->first + i * cache->offset);
	}

	free_pages(p);

	return n;
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
		cache_align = cpu_cache_line_size();
		while (size <= cache_align >> 1)
			cache_align >>= 1;
		align = max(align, cache_align);
	}

	if (align < MIN_ALIGN)
		align = MIN_ALIGN;

	return ALIGN(align, sizeof (void *));
}

/*
 * Calculate how many object will fit on a slab with the given
 * offset between objects.
 */
static int calculate_count(size_t npages, size_t offset, unsigned long flags)
{
	size_t space;
	int n;

	space = npages * PAGE_SIZE;
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
		if (ALIGN(n * sizeof (uint16_t), offset) + n * offset > space)
			--n;
		return n;
	} else {
		return space / offset;
	}
}

static void __init_cache(struct slab_cache *cache, const char *name,
                         size_t size, size_t align, unsigned long flags,
                         void (*ctor)(void *), void (*dtor)(void *))
{
	cache->objsize = size;
	cache->align = calculate_align(flags, align, size);
	cache->offset = ALIGN(size, cache->align);
	/* since the maximum object size is 8192 (2 pages) */
	cache->slab_ord = (size > PAGE_SIZE) ? 1 : 0;

	cache->flags = flags;
	if (size < ON_SLAB_LIMIT)
		cache->flags |= SLAB_DESC_ON_SLAB;

	cache->count = calculate_count(pow2(cache->slab_ord),
	                               cache->offset, cache->flags);

	cache->ctor = ctor;
	cache->dtor = dtor;

	list_init(&cache->full_slabs);
	list_init(&cache->partial_slabs);
	list_init(&cache->free_slabs);
	list_init(&cache->list);

	cache->cache_name[0] = '\0';
	strncat(cache->cache_name, name, NAME_LEN - 1);
}

/*
 * There are a total of 30 caches used by the kmalloc function.
 * They are split into two groups, small and large.
 * The small group consists of caches for all multiples of 8 from 8-192.
 * The large group then contains the remaining powers of 2 from 256 to
 * KMALLOC_MAX_SIZE (8192).
 */
static struct slab_cache *kmalloc_sm_caches[24];
static struct slab_cache *kmalloc_lg_caches[6];

static int kmalloc_active = 0;

/*
 * Initialize all caches used by kmalloc.
 */
void kmalloc_init(void)
{
	struct slab_cache *cache;
	char name[NAME_LEN];
	size_t i, j, sz;
	int err;

	if (kmalloc_active)
		return;

	for (i = 1; i <= 24; ++i) {
		sz = i * 8;
		sprintf(name, "kmalloc-%u", sz);
		cache = create_cache(name, sz, MIN_ALIGN, 0, NULL, NULL);

		for (j = 0; j < 32; ++j) {
			if ((err = grow_cache(cache)))
				goto err_grow;
		}
		kmalloc_sm_caches[i - 1] = cache;
	}

	for (i = 0; i < 6; ++i) {
		sz = 256 * pow2(i);
		sprintf(name, "kmalloc-%u", sz);
		cache = create_cache(name, sz, MIN_ALIGN, 0, NULL, NULL);
		if ((err = grow_cache(cache)))
			goto err_grow;
		if ((err = grow_cache(cache)))
			goto err_grow;
		kmalloc_lg_caches[i] = cache;
	}

	kmalloc_active = 1;
	return;

err_grow:
	panic("failed to grow required cache %s: %s\n",
	      cache->cache_name, strerror(err));
}

static __always_inline struct slab_cache *kmalloc_get_cache(size_t sz)
{
	if (sz <= 192)
		return kmalloc_sm_caches[(sz - 1) / 8];
	else
		return kmalloc_lg_caches[log2(sz - 1) - 7];
}

void *kmalloc(size_t size)
{
	struct slab_cache *cache;

	if (unlikely(!size || size > KMALLOC_MAX_SIZE))
		return NULL;

	cache = kmalloc_get_cache(size);
	return alloc_cache(cache);
}

void kfree(void *ptr)
{
	struct slab_cache *cache;

	cache = virt_to_page(ptr)->slab_cache;
	if (unlikely((uintptr_t)cache == PAGE_UNINIT_MAGIC)) {
		/* klog("attempt to free non-allocated address %lu\n", ptr); */
		return;
	}

	free_cache(cache, ptr);
}
