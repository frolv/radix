/*
 * kernel/mm/vmm.c
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

#include <radix/bits.h>
#include <radix/kernel.h>
#include <radix/mm.h>
#include <radix/slab.h>
#include <radix/vmm.h>

#include <rlibc/stdio.h>
#include <rlibc/string.h>

// A block of virtual addresses in a virtual address space.
//
// When a vmm_block is *not* allocated:
//
//   1. global_list is in the list of all vmm_blocks in the address space.
//   2. area.list is either in the list of all unallocated vmm_blocks of a
//      certain size, or empty. See size_node below.
//   3. size_node is either the position of the block in the tree of unallocated
//      blocks by size, or not in any tree. When it is the latter, there are
//      other vmm_blocks of the same size in the address space, one of which is
//      the tree node, with the rest stored in its area.list.
//   4. addr_node is in the tree of unallocated vmm_blocks sorted by base
//      address.
//   5. allocated_pages is NULL.
//
// When a vmm_block *is* allocated:
//
//   1. global_list is in the list of all vmm_blocks in the address space.
//      (This doesn't change.)
//   2. area.list is in the list of all allocated vmm_blocks in the address
//      space.
//   3. size_node is not used.
//   4. addr_node is in the tree of all allocated vmm_blocks in the address
//      space, sorted by base address.
//   5. allocated_pages is either NULL or a pointer to a struct page
//      representing a block of physical pages allocated for this vmm_block.
//      The struct page's list stores all of the other physical page blocks
//      allocated for this block.
//
struct vmm_block {
    struct vmm_area area;
    struct page *allocated_pages;
    struct vmm_space *vmm;
    uint32_t flags;
    uint32_t pad0;
    struct list global_list;
    struct rb_node size_node;
    struct rb_node addr_node;
};

#define VMM_ALLOCATED (1 << 31)

// Subset of the vmm API flags which is stored in block objects.
#define VMM_BLOCK_FLAGS (VMM_READ | VMM_WRITE | VMM_EXEC)

#define vmm_alloc_block()     alloc_cache(vmm_block_cache)
#define vmm_free_block(block) free_cache(vmm_block_cache, block)

static struct slab_cache *vmm_block_cache;
static struct slab_cache *vmm_space_cache;

static struct vmm_space vmm_kernel = {
    .structures =
        {
            .block_list = LIST_INIT(vmm_kernel.structures.block_list),
            .alloc_list = LIST_INIT(vmm_kernel.structures.alloc_list),
            .addr_tree = RB_ROOT,
            .size_tree = RB_ROOT,
            .alloc_tree = RB_ROOT,
        },
    .vmm_list = LIST_INIT(vmm_kernel.vmm_list),
    .lock = SPINLOCK_INIT,
    .pages = 0,
};

static void vmm_block_init(void *p)
{
    struct vmm_block *block = p;

    block->flags = 0;
    block->allocated_pages = NULL;
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
        new = list_first_entry(&block->area.list, struct vmm_block, area.list);
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

// Finds the smallest unallocated block in an address space which is greater
// than or equal to `size`.
static struct vmm_block *vmm_find_by_size(const struct vmm_space *vmm,
                                          size_t size)
{
    struct rb_node *curr = vmm->structures.size_tree.root_node;
    struct vmm_block *best = NULL;

    while (curr) {
        struct vmm_block *block = rb_entry(curr, struct vmm_block, size_node);

        if (size == block->area.size) {
            return block;
        }

        if (size > block->area.size) {
            curr = curr->right;
        } else {
            if (!best || block->area.size < best->area.size) {
                best = block;
            }
            curr = curr->left;
        }
    }

    return best;
}

// Finds the unallocated block in the provided address space which contains the
// given address, if it exists.
static struct vmm_block *vmm_find_by_addr(const struct vmm_space *vmm,
                                          addr_t addr)
{
    struct rb_node *curr = vmm->structures.addr_tree.root_node;

    while (curr) {
        struct vmm_block *block = rb_entry(curr, struct vmm_block, addr_node);

        if (addr < block->area.base) {
            curr = curr->left;
        } else if (addr >= block->area.base + block->area.size) {
            curr = curr->right;
        } else {
            return block;
        }
    }

    return NULL;
}

// Checks if virtual address `addr` has been allocated in the given vmm space.
static struct vmm_block *vmm_find_allocated(const struct vmm_space *vmm,
                                            addr_t addr)
{
    struct rb_node *curr = vmm->structures.alloc_tree.root_node;

    while (curr) {
        struct vmm_block *block = rb_entry(curr, struct vmm_block, addr_node);

        if (addr < block->area.base) {
            curr = curr->left;
        } else if (addr >= block->area.base + block->area.size) {
            curr = curr->right;
        } else {
            return block;
        }
    }

    return NULL;
}

void vmm_init(void)
{
    struct vmm_block *first;

    vmm_block_cache = create_cache("vmm_block",
                                   sizeof(struct vmm_block),
                                   SLAB_MIN_ALIGN,
                                   SLAB_PANIC,
                                   vmm_block_init);
    vmm_space_cache = create_cache("vmm_space",
                                   sizeof(struct vmm_space),
                                   SLAB_MIN_ALIGN,
                                   SLAB_PANIC,
                                   vmm_space_init);

    first = vmm_alloc_block();
    if (IS_ERR(first))
        panic("failed to allocate intial vmm_block: %s\n",
              strerror(ERR_VAL(first)));

    first->area.base = RESERVED_VIRT_BASE;
    first->area.size = RESERVED_SIZE;
    first->vmm = NULL;

    list_add(&vmm_kernel.structures.block_list, &first->global_list);
    vmm_tree_insert(&vmm_kernel.structures, first);
}

// Splits a single vmm_block into multiple blocks, one of which starts at
// address `base` and is of length `size`. Returns this block.
//
// `block` MUST be large enough to fit `base` + `size`.
static struct vmm_block *vmm_split(struct vmm_block *block,
                                   addr_t base,
                                   size_t size)
{
    assert(ALIGNED(size, PAGE_SIZE));

    struct vmm_block *new;

    struct vmm_structures *s =
        block->vmm ? &block->vmm->structures : &vmm_kernel.structures;
    const size_t before_size = base - block->area.base;
    const addr_t block_end = block->area.base + block->area.size;

    // Create a new block [block->area.base, base).
    if (before_size > 0) {
        new = vmm_alloc_block();
        if (IS_ERR(new)) {
            return new;
        }

        block->area.size = before_size;
        vmm_size_tree_delete(&s->size_tree, block);
        vmm_size_tree_insert(&s->size_tree, block);

        new->area.base = base;
        new->area.size = size;
        new->vmm = block->vmm;
        list_add(&block->global_list, &new->global_list);
        block = new;
    } else if (size != block->area.size) {
        // base == block->area.base
        block->area.size = size;
        vmm_tree_delete(s, block);
    }

    const size_t after_size = block_end - (block->area.base + block->area.size);

    // Create a new block [block->area.base + block->area.size, end).
    if (after_size > 0) {
        new = vmm_alloc_block();
        if (IS_ERR(new)) {
            block->area.size += after_size;
            vmm_tree_insert(s, block);
            return new;
        }

        new->area.base = block->area.base + block->area.size;
        new->area.size = after_size;
        new->vmm = block->vmm;
        list_add(&block->global_list, &new->global_list);
        vmm_tree_insert(s, new);
    }

    return block;
}

// Attempt to merge `block` with its unallocated neighbors to form a larger
// vmm_block, and return the new block.
static struct vmm_block *vmm_try_coalesce(struct vmm_block *block)
{
    struct vmm_block *neighbor;
    addr_t new_base;
    size_t new_size;

    struct vmm_space *vmm = block->vmm ? block->vmm : &vmm_kernel;
    struct vmm_structures *s = &vmm->structures;

    spin_lock(&vmm->lock);

    rb_delete(&s->addr_tree, &block->addr_node);
    list_del(&block->area.list);
    block->flags &= ~(VMM_ALLOCATED | VMM_BLOCK_FLAGS);

    new_base = block->area.base;
    new_size = block->area.size;

    // Merge with lower address blocks.
    while (block->global_list.prev != &s->block_list) {
        neighbor = list_prev_entry(block, global_list);
        if (neighbor->flags & VMM_ALLOCATED) {
            break;
        }

        new_base = neighbor->area.base;
        new_size += neighbor->area.size;

        list_del(&neighbor->global_list);
        vmm_tree_delete(s, neighbor);
        vmm_free_block(neighbor);
    }

    // Merge with higher address blocks.
    while (block->global_list.next != &s->block_list) {
        neighbor = list_next_entry(block, global_list);
        if (neighbor->flags & VMM_ALLOCATED) {
            break;
        }

        new_size += neighbor->area.size;

        list_del(&neighbor->global_list);
        vmm_tree_delete(s, neighbor);
        vmm_free_block(neighbor);
    }

    block->area.base = new_base;
    block->area.size = new_size;
    vmm_tree_insert(s, block);

    spin_unlock(&vmm->lock);

    return block;
}

// Allocates physical pages for an entire virtual range of kernel address space.
// Note: This is generally a bad idea.
static void vmm_alloc_block_pages(struct vmm_block *block)
{
    // TODO(frolv): Don't use NULL to represent the kernel address space.
    assert(block->vmm == NULL);

    addr_t base = block->area.base;
    const addr_t end = block->area.base + block->area.size;
    int pages = block->area.size / PAGE_SIZE;

    while (base < end) {
        int ord = min(log2(pages), PA_MAX_ORDER);
        struct page *p = alloc_pages(PA_USER, ord);

        // It's okay if this fails; there will be a second chance when the page
        // fault handler is hit.
        if (IS_ERR(p)) {
            return;
        }

        map_pages_kernel(
            base, page_to_phys(p), pow2(ord), PROT_WRITE, PAGE_CP_DEFAULT);
        vmm_add_area_pages(&block->area, p);

        pages -= pow2(ord);
        base += pow2(ord) * PAGE_SIZE;
    }
}

static void free_pages_refcount(struct page *p)
{
    PM_REFCOUNT_DEC(p);
    if (PM_PAGE_REFCOUNT(p) == 0) {
        free_pages(p);
    }
}

static void vmm_free_pages(struct vmm_block *block)
{
    if (!block->allocated_pages) {
        return;
    }

    struct vmm_space *vmm = block->vmm ? block->vmm : &vmm_kernel;

    while (!list_empty(&block->allocated_pages->list)) {
        struct page *p =
            list_first_entry(&block->allocated_pages->list, struct page, list);
        vmm->pages -= pow2(PM_PAGE_BLOCK_ORDER(p));
        list_del(&p->list);
        free_pages_refcount(p);
    }

    vmm->pages -= pow2(PM_PAGE_BLOCK_ORDER(block->allocated_pages));
    free_pages_refcount(block->allocated_pages);
    block->allocated_pages = NULL;
}

struct vmm_space *vmm_new(void)
{
    struct vmm_space *vmm = alloc_cache(vmm_space_cache);
    if (IS_ERR(vmm)) {
        return NULL;
    }

    struct vmm_block *initial = vmm_alloc_block();
    if (IS_ERR(initial)) {
        free_cache(vmm_space_cache, vmm);
        return NULL;
    }

    // Set up the initial block encompassing the entire user address space.
    initial->area.base = USER_VIRTUAL_BASE;
    initial->area.size = USER_VIRTUAL_SIZE;
    initial->vmm = vmm;

    list_add(&vmm->structures.block_list, &initial->global_list);
    vmm_tree_insert(&vmm->structures, initial);

    arch_vmm_setup(vmm);

    return vmm;
}

void vmm_release(struct vmm_space *vmm)
{
    assert(spin_try_lock(&vmm->lock));

    arch_vmm_release(vmm);

    // Free all the blocks in the space. No need to remove them from the trees
    // as the vmm_space is being freed directly afterwards.
    while (!list_empty(&vmm->structures.block_list)) {
        struct vmm_block *block = list_first_entry(
            &vmm->structures.block_list, struct vmm_block, global_list);
        list_del(&block->global_list);
        vmm_free_pages(block);
        vmm_free_block(block);
    }

    free_cache(vmm_space_cache, vmm);
}

struct vmm_area *vmm_alloc_size(struct vmm_space *vmm,
                                size_t size,
                                uint32_t flags)
{
    if (!vmm) {
        vmm = &vmm_kernel;
    }

    unsigned long irqstate;
    int err;

    size = ALIGN(size, PAGE_SIZE);
    spin_lock_irq(&vmm->lock, &irqstate);

    struct vmm_block *block = vmm_find_by_size(vmm, size);
    if (!block) {
        err = ENOMEM;
        goto out_err;
    }

    addr_t base = block->area.base + block->area.size - size;

    block = vmm_split(block, base, size);
    if (IS_ERR(block)) {
        err = ERR_VAL(block);
        goto out_err;
    }

    uint32_t block_flags = flags & VMM_BLOCK_FLAGS;
    block->flags |= (VMM_ALLOCATED | block_flags);

    list_ins(&vmm->structures.alloc_list, &block->area.list);
    vmm_addr_tree_insert(&vmm->structures.alloc_tree, block);

    spin_unlock_irq(&vmm->lock, irqstate);

    if (flags & VMM_ALLOC_UPFRONT) {
        vmm_alloc_block_pages(block);
    }

    return &block->area;

out_err:
    spin_unlock_irq(&vmm->lock, irqstate);
    return ERR_PTR(err);
}

struct vmm_area *vmm_alloc_addr(struct vmm_space *vmm,
                                addr_t addr,
                                size_t size,
                                uint32_t flags)
{
    if (!vmm) {
        vmm = &vmm_kernel;
    }

    unsigned long irqstate;

    size = ALIGN(size, PAGE_SIZE);
    addr &= PAGE_MASK;
    spin_lock_irq(&vmm->lock, &irqstate);

    struct vmm_block *block = vmm_find_by_addr(vmm, addr);
    if (!block) {
        spin_unlock_irq(&vmm->lock, irqstate);
        return ERR_PTR(ENOMEM);
    }

    const addr_t block_end = block->area.base + block->area.size;
    if (addr + size > block_end) {
        spin_unlock_irq(&vmm->lock, irqstate);
        return ERR_PTR(ENOMEM);
    }

    block = vmm_split(block, addr, size);
    if (IS_ERR(block)) {
        spin_unlock_irq(&vmm->lock, irqstate);
        return ERR_PTR(ERR_VAL(block));
    }

    uint32_t block_flags = flags & VMM_BLOCK_FLAGS;
    block->flags |= (VMM_ALLOCATED | block_flags);

    list_ins(&vmm->structures.alloc_list, &block->area.list);
    vmm_addr_tree_insert(&vmm->structures.alloc_tree, block);

    spin_unlock_irq(&vmm->lock, irqstate);
    return &block->area;
}

void vmm_free(struct vmm_area *area)
{
    struct vmm_block *block;

    block = (struct vmm_block *)area;
    if (!(block->flags & VMM_ALLOCATED)) {
        return;
    }

    vmm_free_pages(block);
    vmm_try_coalesce(block);
}

void *vmalloc(size_t size)
{
    struct vmm_area *area = vmm_alloc_size(NULL, size, VMM_READ | VMM_WRITE);
    if (IS_ERR(area)) {
        return NULL;
    }

    return (void *)area->base;
}

void vfree(void *ptr)
{
    struct vmm_block *block = vmm_find_allocated(&vmm_kernel, (addr_t)ptr);
    if (block) {
        vmm_free_pages(block);
    }
}

// Checks if `addr` is allocated in address space `vmm`, and returns its
// vmm_area if so.
struct vmm_area *vmm_get_allocated_area(struct vmm_space *vmm, addr_t addr)
{
    struct vmm_block *block = vmm_find_allocated(vmm ? vmm : &vmm_kernel, addr);
    return (struct vmm_area *)block;
}

// Adds the block of physical pages represented by `p` to `area`.
void vmm_add_area_pages(struct vmm_area *area, struct page *p)
{
    struct vmm_block *block = (struct vmm_block *)area;
    struct vmm_space *vmm = block->vmm;

    if (vmm) {
        vmm->pages += pow2(PM_PAGE_BLOCK_ORDER(p));
    }

    if (vmm == &vmm_kernel) {
        p->mem = (void *)area->base;
        p->status |= PM_PAGE_MAPPED;
    }

    PM_REFCOUNT_INC(p);

    if (!block->allocated_pages) {
        block->allocated_pages = p;
    } else {
        list_ins(&block->allocated_pages->list, &p->list);
    }
}

int vmm_map_pages(struct vmm_area *area, addr_t addr, struct page *p)
{
    int pages = pow2(PM_PAGE_BLOCK_ORDER(p));

    if (addr < area->base ||
        addr + pages * PAGE_SIZE > area->base + area->size) {
        return EINVAL;
    }

    const struct vmm_block *block = (const struct vmm_block *)area;

    int prot = 0;
    if (block->flags & VMM_READ) {
        prot |= PROT_READ;
    }
    if (block->flags & VMM_WRITE) {
        prot |= PROT_WRITE;
    }
    if (block->flags & VMM_EXEC) {
        prot |= PROT_EXEC;
    }

    int err = map_pages_vmm(
        block->vmm, addr, page_to_phys(p), pages, prot, PAGE_CP_DEFAULT);
    if (err) {
        return err;
    }

    vmm_add_area_pages(area, p);
    return 0;
}

void vmm_space_dump(struct vmm_space *vmm)
{
    struct vmm_structures *s = vmm ? &vmm->structures : &vmm_kernel.structures;
    int i = 0;

    printf("vmm_space:\n");
    printf("idx\tvirtual range\t\tflags\n");

    struct vmm_block *block;
    list_for_each_entry (block, &s->block_list, global_list) {
        char flags[5];
        strcpy(flags, "----");

        if (block->flags & VMM_ALLOCATED) {
            flags[0] = 'A';
        }
        if (block->flags & VMM_READ) {
            flags[1] = 'R';
        }
        if (block->flags & VMM_WRITE) {
            flags[2] = 'W';
        }
        if (block->flags & VMM_EXEC) {
            flags[3] = 'X';
        }

        printf("%d\t%p-%p\t[%s]\n",
               i++,
               block->area.base,
               block->area.base + block->area.size,
               flags);
    }
}

static void vmm_page_dump(struct page *p)
{
    addr_t phys;
    size_t npages;

    phys = page_to_phys(p);
    npages = pow2(PM_PAGE_BLOCK_ORDER(p));

    printf("%p-%p [%3d page%c]\n",
           phys,
           phys + npages * PAGE_SIZE,
           npages,
           npages > 1 ? 's' : ' ');
}

void vmm_block_dump(struct vmm_block *block)
{
    struct page *p;

    printf("vmm_block:\n%p-%p [%u KiB]\n",
           block->area.base,
           block->area.base + block->area.size,
           (block->area.base + block->area.size) / KIB(1));

    if (!(p = block->allocated_pages)) {
        return;
    }

    vmm_page_dump(p);
    list_for_each_entry (p, &p->list, list) {
        vmm_page_dump(p);
    }
}
