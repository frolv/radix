/*
 * kernel/mm/vmm.c
 * Copyright (C) 2017 Alexei Frolov
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

#include <radix/bits.h>
#include <radix/kernel.h>
#include <radix/mm.h>
#include <radix/slab.h>
#include <radix/vmm.h>

#include <rlibc/stdio.h>
#include <rlibc/string.h>

struct vmm_block {
	struct vmm_area         area;
	struct page             *mapped;
	struct vmm_space        *vmm;
	unsigned long           flags;
	unsigned long           padding;
	struct list             global_list;
	struct rb_node          size_node;
	struct rb_node          addr_node;
};

/*
 * Alright. There's a bit of list/tree stuff going on here, so listen carefully.
 *
 * When a vmm_block is *not* allocated:
 * 1. global_list is in the list of all vmm_blocks in the address space.
 * 2. area.list is either in the list of all unallocated vmm_blocks of a
 *    certain size, or empty. See size_node below.
 * 3. size_node is either the position of the block in the tree of unallocated
 *    blocks by size, or not in any tree. When it is the latter, there are other
 *    vmm_blocks of the same size in the address space, one of which is the tree
 *    node, with the rest stored in its area.list.
 * 4. addr_node is in the tree of unallocated vmm_blocks sorted by base address.
 * 5. mapped is NULL.
 *
 * When a vmm_block *is* allocated:
 * 1. global_list is in the list of all vmm_blocks in the address space.
 *    (This doesn't change.)
 * 2. area.list is in the list of all allocated vmm_blocks in the address space.
 * 3. size_node is not used.
 * 4. addr_node is in the tree of all allocated vmm_blocks in the address space,
 *    sorted by base address.
 * 5. mapped is either NULL or a pointer to a struct page representing a group
 *    of physical pages allocated for this vmm_block. The struct page's list
 *    stores all of the other physical page groups allocated for this block.
 */

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
			list_ins(&curr->area.list, &block->area.list);
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

	if (!list_empty(&block->area.list)) {
		new = list_first_entry(&block->area.list,
		                       struct vmm_block, area.list);
		rb_replace(tree, &block->size_node, &new->size_node);
		list_del(&block->area.list);
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
 * Find the smallest block in `s->size_tree` which is greater than
 * or equal to `size`.
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

/*
 * vmm_find_addr:
 * Check if virtual address `addr` has been allocated in the given
 * address space.
 */
static struct vmm_block *vmm_find_addr(struct vmm_structures *s, addr_t addr)
{
	struct vmm_block *block;
	struct rb_node *curr;

	curr = s->alloc_tree.root_node;

	while (curr) {
		block = rb_entry(curr, struct vmm_block, addr_node);

		if (addr < block->area.base)
			curr = curr->left;
		else if (addr > block->area.base + block->area.size)
			curr = curr->right;
		else
			return block;
	}

	return NULL;
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
	first->vmm = NULL;

	list_add(&vmm_kernel.block_list, &first->global_list);
	vmm_tree_insert(&vmm_kernel, first);

	arch_prepare_pf();
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
                                   struct vmm_space *vmm,
                                   addr_t base, size_t size)
{
	struct vmm_structures *s;
	struct vmm_block *new;
	size_t new_size, end;

	s = vmm ? &vmm->structures : &vmm_kernel;
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
		new->vmm = vmm;
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
		new->vmm = vmm;
		list_add(&block->global_list, &new->global_list);
		vmm_tree_insert(s, new);
	}

	return block;
}

static __always_inline void __vmm_add_area_pages(struct vmm_block *block,
                                                 struct page *p)
{
	if (!block->mapped)
		block->mapped = p;
	else
		list_ins(&block->mapped->list, &p->list);
}

/*
 * vmm_alloc_block_pages:
 * Allocate physical pages for the whole address range of the given vmm_block.
 * Note: this is almost always a _bad_ idea.
 */
static void vmm_alloc_block_pages(struct vmm_block *block)
{
	struct page *p;
	addr_t base, end;
	size_t ord;
	int pages;

	base = block->area.base;
	end = block->area.base + block->area.size;
	pages = block->area.size / PAGE_SIZE;

	while (base < end) {
		ord = min(log2(pages), PA_MAX_ORDER);

		p = alloc_pages(PA_USER, ord);
		/*
		 * It's OK if this fails; there will be a second chance
		 * when the page fault handler is hit.
		 */
		if (IS_ERR(p))
			return;

		map_pages_kernel(base, page_to_phys(p), PROT_WRITE,
		                 PAGE_CP_DEFAULT, pow2(ord));
		p->mem = (void *)base;
		__vmm_add_area_pages(block, p);

		pages -= pow2(ord);
		base += pow2(ord) * PAGE_SIZE;
	}
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
	block = vmm_split(block, NULL, base, size);
	if (IS_ERR(block)) {
		err = ERR_VAL(block);
		goto out_err;
	}

	block->flags |= VMM_ALLOCATED;
	list_ins(&vmm_kernel.alloc_list, &block->area.list);
	vmm_addr_tree_insert(&vmm_kernel.alloc_tree, block);
	/* TODO: unlock vmm_kernel_lock */

	if (flags & VMM_ALLOC_UPFRONT)
		vmm_alloc_block_pages(block);

	return &block->area;

out_err:
	/* TODO: unlock vmm_kernel_lock */
	return ERR_PTR(err);
}

static struct vmm_area *__vmm_alloc_size(struct vmm_space *vmm, size_t size,
                                         unsigned long flags)
{
	if (flags & VMM_ALLOC_UPFRONT)
		return ERR_PTR(EINVAL);

	(void)vmm;
	(void)size;

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

static void __vmm_free_pages(struct vmm_space *vmm, struct vmm_block *block)
{
	struct page *p;

	if (!block->mapped)
		return;

	while (!list_empty(&block->mapped->list)) {
		p = list_first_entry(&block->mapped->list, struct page, list);
		vmm->pages -= pow2(PM_PAGE_BLOCK_ORDER(p));
		list_del(&p->list);
		free_pages(p);
	}
	vmm->pages -= pow2(PM_PAGE_BLOCK_ORDER(block->mapped));
	free_pages(block->mapped);
	block->mapped = NULL;
}

static void __vmm_free_kernel_pages(struct vmm_block *block)
{
	struct page *p;

	if (!block->mapped)
		return;

	while (!list_empty(&block->mapped->list)) {
		p = list_first_entry(&block->mapped->list, struct page, list);
		list_del(&p->list);
		free_pages(p);
	}
	free_pages(block->mapped);
	block->mapped = NULL;
}

/* vmm_free: free the vmm_area `area` */
void vmm_free(struct vmm_space *vmm, struct vmm_area *area)
{
	struct vmm_block *block;
	struct vmm_structures *s;

	block = (struct vmm_block *)area;
	if (!(block->flags & VMM_ALLOCATED))
		return;

	if (vmm) {
		s = &vmm->structures;
		__vmm_free_pages(vmm, block);
	} else {
		s = &vmm_kernel;
		__vmm_free_kernel_pages(block);
	}

	rb_delete(&s->addr_tree, &block->addr_node);
	list_del(&block->area.list);
	block->flags &= ~VMM_ALLOCATED;
	vmm_tree_insert(s, block);
}

void *vmalloc(size_t size)
{
	struct vmm_area *area;

	area = vmm_alloc_size(NULL, size, 0);
	if (IS_ERR(area))
		return NULL;

	return (void *)area->base;
}

/*
 * vmm_get_allocated_area:
 * Check if `addr` is allocated in address space `vmm`,
 * and return its vmm_area if so.
 */
struct vmm_area *vmm_get_allocated_area(struct vmm_space *vmm, addr_t addr)
{
	struct vmm_block *block;

	block = vmm_find_addr(vmm ? &vmm->structures : &vmm_kernel, addr);
	return (struct vmm_area *)block;
}

/*
 * vmm_add_area_pages:
 * Add the block of physical pages represented by `p` to `area`.
 */
void vmm_add_area_pages(struct vmm_area *area, struct page *p)
{
	struct vmm_block *block;
	struct vmm_space *vmm;

	block = (struct vmm_block *)area;
	vmm = block->vmm;
	if (vmm)
		vmm->pages += pow2(PM_PAGE_BLOCK_ORDER(p));

	__vmm_add_area_pages(block, p);
}

void vmm_space_dump(struct vmm_space *vmm)
{
	struct vmm_structures *s;
	struct vmm_block *block;
	struct list *l;
	int i;

	s = vmm ? &vmm->structures : &vmm_kernel;
	i = 0;

	printf("vmm_space:\n");
	list_for_each(l, &s->block_list) {
		block = list_entry(l, struct vmm_block, global_list);
		printf("%d %p-%p [%c]\n",
		       i++, block->area.base,
		       block->area.base + block->area.size,
		       block->flags & VMM_ALLOCATED ? 'A' : '-');
	}
}

static void vmm_page_dump(struct page *p)
{
	addr_t phys;
	size_t npages;

	phys = page_to_phys(p);
	npages = pow2(PM_PAGE_BLOCK_ORDER(p));

	printf("%p-%p [%3d page%c]\n",
	       phys, phys + npages * PAGE_SIZE,
	       npages, npages > 1 ? 's' : ' ');
}

void vmm_block_dump(struct vmm_block *block)
{
	struct page *p;
	struct list *l;

	printf("vmm_block:\n%p-%p [%u KiB]\n",
	       block->area.base,
	       block->area.base + block->area.size,
	       (block->area.base + block->area.size) / KIB(1));

	if (!(p = block->mapped))
		return;

	vmm_page_dump(p);
	list_for_each(l, &p->list)
		vmm_page_dump(list_entry(l, struct page, list));
}
