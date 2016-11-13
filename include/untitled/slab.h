/*
 * include/untitled/slab.h
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

#ifndef UNTITLED_SLAB_H
#define UNTITLED_SLAB_H

#include <stddef.h>
#include <untitled/list.h>

#define NAME_LEN	0x40

struct slab_cache {
	struct list	full;			/* full slabs */
	struct list	partial;		/* partially full slabs */
	struct list	free;			/* empty slabs */
	size_t		objsize;		/* size of each cached object */
	int		count;			/* number of objects per slab */
	int		flags;			/* allocator options */
	char		cache_name[NAME_LEN];	/* human-readable cache name */
	struct list	list;			/* list of caches */
};

struct slab_cache *create_cache(const char *name, size_t size, size_t offset);
void *alloc_cache(struct slab_cache *cache);

#define KMALLOC_MAX_SIZE 0x2000

#endif /* UNTITLED_SLAB_H */
