/*
 * include/radix/rbtree.h
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

#ifndef RADIX_RBTREE_H
#define RADIX_RBTREE_H

#include <radix/compiler.h>
#include <radix/types.h>

struct rb_node {
    unsigned long __parent;
    struct rb_node *left;
    struct rb_node *right;
};

struct rb_root {
    struct rb_node *root_node;
};

#define RB_ROOT \
    (struct rb_root) { NULL }

#define __rb_parent_addr(node) ((node)->__parent & ~3UL)
#define rb_parent(node)        ((struct rb_node *)__rb_parent_addr(node))

#define rb_entry(ptr, type, member) container_of(ptr, type, member)

/* rb_init: initialize a red-black tree node */
static __always_inline void rb_init(struct rb_node *node)
{
    node->__parent = (unsigned long)node;
    node->left = node->right = NULL;
}

/*
 * rb_link: link `node` as the child of parent.
 * Must be called only when adding a new node to a tree.
 */
static __always_inline void rb_link(struct rb_node *node,
                                    struct rb_node *parent,
                                    struct rb_node **pos)
{
    *pos = node;
    node->__parent = (unsigned long)parent;
}

void rb_balance(struct rb_root *root, struct rb_node *node);
void rb_delete(struct rb_root *root, struct rb_node *node);
void rb_replace(struct rb_root *root, struct rb_node *old, struct rb_node *new);

#endif /* RADIX_RBTREE_H */
