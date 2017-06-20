/*
 * kernel/mm/vmm.c
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

#include <radix/kernel.h>
#include <radix/mm.h>
#include <radix/slab.h>
#include <radix/vmm.h>

#include <rlibc/string.h>

struct vmm_block {
	struct vmm_area area;
	unsigned long   flags;
	struct page     *mapped;
	struct list     global_list;
	struct list     size_list;
	struct rb_node  size_node;
	struct rb_node  addr_node;
};

#define VMM_ALLOCATED (1 << 0)

static struct slab_cache *vmm_block_cache;
static struct slab_cache *vmm_space_cache;

static struct vmm_structures vmm_kernel = {
	.block_list = LIST_INIT(vmm_kernel.block_list),
	.alloc_list = LIST_INIT(vmm_kernel.alloc_list),
	.addr_tree = RB_ROOT,
	.size_tree = RB_ROOT,
	.alloc_tree = RB_ROOT
};

static void vmm_block_init(void *p)
{
	struct vmm_block *block = p;

	block->flags = 0;
	block->mapped = NULL;
	list_init(&block->area.list);
	list_init(&block->global_list);
	list_init(&block->size_list);
	rb_init(&block->size_node);
	rb_init(&block->addr_node);
}

static void vmm_structures_init(struct vmm_structures *s)
{
	list_init(&s->block_list);
	list_init(&s->alloc_list);
	s->addr_tree = RB_ROOT;
	s->size_tree = RB_ROOT;
	s->alloc_tree = RB_ROOT;
}

static void vmm_space_init(void *p)
{
	struct vmm_space *vmm = p;

	vmm_structures_init(&vmm->structures);
	list_init(&vmm->vmm_list);
	vmm->pages = 0;
}

/*
 * vmm_size_tree_insert:
 * Insert `block` into given VMM block size tree.
 */
static void vmm_size_tree_insert(struct rb_root *tree, struct vmm_block *block)
{
	struct rb_node **pos, *parent;
	struct vmm_block *curr;

	pos = &tree->root_node;
	parent = NULL;

	while (*pos) {
		curr = rb_entry(*pos, struct vmm_block, size_node);
		parent = *pos;

		if (block->area.size < curr->area.size) {
			pos = &(*pos)->left;
		} else if (block->area.size > curr->area.size) {
			pos = &(*pos)->right;
		} else {
			list_ins(&curr->size_list, &block->size_list);
			return;
		}
	}

	rb_link(&block->size_node, parent, pos);
	rb_balance(tree, &block->size_node);
}

/*
 * vmm_size_tree_delete:
 * Delete `block` from the given VMM block size tree.
 */
static __always_inline void vmm_size_tree_delete(struct rb_root *tree,
                                                 struct vmm_block *block)
{
	struct vmm_block *new;

	if (!list_empty(&block->size_list)) {
		new = list_first_entry(&block->size_list,
		                       struct vmm_block, size_list);
		rb_replace(tree, &block->size_node, &new->size_node);
		list_del(&block->size_list);
	} else {
		rb_delete(tree, &block->size_node);
	}
}

/*
 * vmm_addr_tree_insert:
 * Insert `block` into given VMM block address tree.
 */
static void vmm_addr_tree_insert(struct rb_root *tree, struct vmm_block *block)
{
	struct rb_node **pos, *parent;
	struct vmm_block *curr;

	pos = &tree->root_node;
	parent = NULL;

	while (*pos) {
		curr = rb_entry(*pos, struct vmm_block, addr_node);
		parent = *pos;

		if (block->area.base < curr->area.base)
			pos = &(*pos)->left;
		else if (block->area.base > curr->area.base)
			pos = &(*pos)->right;
		else
			return;
	}

	rb_link(&block->addr_node, parent, pos);
	rb_balance(tree, &block->addr_node);
}

/*
 * vmm_tree_insert:
 * Insert a vmm_block into both address and size trees.
 */
static __always_inline void vmm_tree_insert(struct vmm_structures *s,
                                            struct vmm_block *block)
{
	vmm_addr_tree_insert(&s->addr_tree, block);
	vmm_size_tree_insert(&s->size_tree, block);
}

/*
 * vmm_tree_delete:
 * Delete a vmm_block from both address and size trees.
 */
static __always_inline void vmm_tree_delete(struct vmm_structures *s,
                                            struct vmm_block *block)
{
	rb_delete(&s->addr_tree, &block->addr_node);
	vmm_size_tree_delete(&s->size_tree, block);
}

/*
 * vmm_find_by_size:
 * Find the smallest block in `s` which is greater than or equal to `size`.
 */
static struct vmm_block *vmm_find_by_size(struct vmm_structures *s, size_t size)
{
	struct vmm_block *block, *best;
	struct rb_node *curr;

	curr = s->size_tree.root_node;
	best = NULL;

	while (curr) {
		block = rb_entry(curr, struct vmm_block, size_node);

		if (size == block->area.size) {
			return block;
		} else if (size > block->area.size) {
			curr = curr->right;
		} else {
			if (!best || block->area.size < best->area.size)
				best = block;

			curr = curr->left;
		}
	}

	return best;
}

void vmm_init(void)
{
	struct vmm_block *first;

	vmm_block_cache = create_cache("vmm_block", sizeof (struct vmm_block),
	                               SLAB_MIN_ALIGN, SLAB_PANIC,
	                               vmm_block_init, vmm_block_init);
	vmm_space_cache = create_cache("vmm_space", sizeof (struct vmm_space),
	                               SLAB_MIN_ALIGN, SLAB_PANIC,
	                               vmm_space_init, vmm_space_init);

	first = alloc_cache(vmm_block_cache);
	if (IS_ERR(first))
		panic("failed to allocate intial vmm_block: %s\n",
		      strerror(ERR_VAL(first)));

	first->area.base = RESERVED_VIRT_BASE;
	first->area.size = RESERVED_SIZE;
	first->flags = 0;

	list_add(&vmm_kernel.block_list, &first->global_list);
	vmm_tree_insert(&vmm_kernel, first);
}

/*
 * vmm_split:
 * Split a single vmm_block into multiple blocks, one of which
 * starts at address `base` and is of length `size`.
 * Return this block.
 *
 * `block` MUST be large enough to fit `base` + `size`.
 */
static struct vmm_block *vmm_split(struct vmm_block *block,
                                   struct vmm_structures *s,
                                   addr_t base, size_t size)
{
	struct vmm_block *new;
	size_t new_size, end;

	new_size = base - block->area.base;
	end = block->area.base + block->area.size;

	/* create a new block [block->area.base, base) */
	if (new_size) {
		new = alloc_cache(vmm_block_cache);
		if (IS_ERR(new))
			return new;

		block->area.size = new_size;
		vmm_size_tree_delete(&s->size_tree, block);
		vmm_size_tree_insert(&s->size_tree, block);

		new->area.base = base;
		new->area.size = size;
		list_add(&block->global_list, &new->global_list);
		block = new;
	} else if (size != block->area.size) {
		/* base == block->area.base */
		block->area.size = size;
		vmm_tree_delete(s, block);
	}

	new_size = end - (block->area.base + block->area.size);

	/* create a new block [block->area.base + block->area.size, end) */
	if (new_size) {
		new = alloc_cache(vmm_block_cache);
		if (IS_ERR(new)) {
			block->area.size += new_size;
			vmm_tree_insert(s, block);
			return new;
		}

		new->area.base = block->area.base + block->area.size;
		new->area.size = new_size;
		list_add(&block->global_list, &new->global_list);
		vmm_tree_insert(s, new);
	}

	return block;
}

static struct vmm_area *vmm_alloc_size_kernel(size_t size, unsigned long flags)
{
	struct vmm_block *block;
	addr_t base;
	int err;

	/* TODO: lock vmm_kernel_lock */
	block = vmm_find_by_size(&vmm_kernel, size);
	if (!block) {
		err = ENOMEM;
		goto out_err;
	}

	base = block->area.base + block->area.size - size;
	block = vmm_split(block, &vmm_kernel, base, size);
	if (IS_ERR(block)) {
		err = ERR_VAL(block);
		goto out_err;
	}

	block->flags |= VMM_ALLOCATED;
	list_ins(&vmm_kernel.alloc_list, &block->area.list);
	vmm_addr_tree_insert(&vmm_kernel.alloc_tree, block);
	/* TODO: unlock vmm_kernel_lock */

	if (flags & VMM_ALLOC_UPFRONT) {
		/* TODO */
	}

	return (struct vmm_area *)block;

out_err:
	/* TODO: unlock vmm_kernel_lock */
	return ERR_PTR(err);
}

static struct vmm_area *__vmm_alloc_size(struct vmm_space *vmm, size_t size,
                                         unsigned long flags)
{
	(void)vmm;
	(void)size;
	(void)flags;

	return ERR_PTR(ENOMEM);
}

/*
 * vmm_alloc_size:
 * Allocate a block of virtual pages of at least `size`
 * from the address space specified by `vmm`.
 */
struct vmm_area *vmm_alloc_size(struct vmm_space *vmm, size_t size,
                                unsigned long flags)
{
	size = ALIGN(size, PAGE_SIZE);

	if (!vmm)
		return vmm_alloc_size_kernel(size, flags);
	else
		return __vmm_alloc_size(vmm, size, flags);
}

void *vmalloc(size_t size)
{
	struct vmm_area *area;

	area = vmm_alloc_size(NULL, size, 0);
	if (IS_ERR(area))
		return NULL;

	return (void *)area->base;
}
