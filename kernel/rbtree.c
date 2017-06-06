/*
 * kernel/rbtree.c
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

#include <radix/rbtree.h>

#define RB_BLACK 0
#define RB_RED   1

#define rb_colour(node) ((node)->__parent & 1)
#define rb_set_colour(node, colour) \
	((node)->__parent = (__rb_parent_addr(node)) | (colour))
#define rb_set_parent(node, parent) \
	((node)->__parent = ((unsigned long)parent) | rb_colour(node))

/*
 * Red-black tree properties, from Wikipedia:
 * 1. All nodes are either red or black.
 * 2. The root is black.
 * 3. All NULL leaves are black.
 * 4. If a node is red, both its children are black.
 * 5. Every path from a node to any of its descendant NULL
 *    nodes contains the same number of black nodes.
 */

/* rb_rotate_left: perform a left rotation around `node` */
static __always_inline void rb_rotate_left(struct rb_root *root,
                                           struct rb_node *node)
{
	struct rb_node *p, *q, **rr;

	p = node->right;

	if (node == root->root_node) {
		/* `p` becomes the new root */
		rr = &root->root_node;
		p->__parent = 0;
	} else {
		q = rb_parent(node);
		rr = (node == q->left) ? &q->left : &q->right;
		rb_set_parent(p, q);
	}

	node->right = p->left;
	p->left = node;
	*rr = p;

	if (node->right)
		rb_set_parent(node->right, node);
	rb_set_parent(p->left, p);
}

/* rb_rotate_right: perform a right rotation around non-root `node` */
static __always_inline void rb_rotate_right(struct rb_root *root,
                                            struct rb_node *node)
{
	struct rb_node *p, *q, **rr;

	p = node->left;

	if (node == root->root_node) {
		/* `p` becomes the new root */
		rr = &root->root_node;
		p->__parent = 0;
	} else {
		q = rb_parent(node);
		rr = (node == q->left) ? &q->left : &q->right;
		rb_set_parent(p, q);
	}

	node->left = p->right;
	p->right = node;
	*rr = p;

	if (node->left)
		rb_set_parent(node->left, node);
	rb_set_parent(p->right, p);
}

/*
 * rb_balance:
 * Balance tree around newly inserted node `node`.
 *
 * The behaviour of this function is undefined if rb_link
 * has not been properly called on `node` prior to it.
 */
void rb_balance(struct rb_root *root, struct rb_node *node)
{
	struct rb_node *pa, *un, *gp;

	if (unlikely(!node || !root))
		return;

	/* case 1: node is root. Set its parent to NULL and colour black. */
	if (node == root->root_node) {
		node->__parent = 0;
		return;
	}

	rb_set_colour(node, RB_RED);
	pa = rb_parent(node);

	/* case 2: node's parent is black. The tree is valid. */
	if (rb_colour(pa) == RB_BLACK)
		return;

	/*
	 * At this point, we know that `node` has a grandparent.
	 * If it didn't, its parent would be the root, which is black.
	 */
	gp = rb_parent(pa);
	un = (pa == gp->left) ? gp->right : gp->left;

	/*
	 * case 3: both parent and uncle are red.
	 * Make them both black and the grandparent red to maintain
	 * property 5, then rebalance around the modified grandparent.
	 * Note that setting the grandparent red is done at the start
	 * of the recursive rb_balance call.
	 */
	if (un && rb_colour(un) == RB_RED) {
		rb_set_colour(pa, RB_BLACK);
		rb_set_colour(un, RB_BLACK);
		rb_balance(root, gp);
		return;
	}

	/*
	 * case 4: parent is red and uncle is black, and `node` and its
	 *         parent are opposite children.
	 * A rotation is performed and the roles of `node` and its parent
	 * are switched to set up case 5. This rotation does not violate
	 * property 5 because both `node` and its parent are red.
	 */
	if (node == pa->right && pa == gp->left) {
		rb_rotate_left(root, pa);
		pa = node;
		node = node->left;
	} else if (node == pa->left && pa == gp->right) {
		rb_rotate_right(root, pa);
		pa = node;
		node = node->right;
	}

	/*
	 * case 5: parent is red and and uncle is black, and `node` and its
	 *         parent are children on the same side.
	 * Rotate around parent around grandparent and swap their colours.
	 * This satisfies property 4, as both children of grandparent will be
	 * black, and property 5, as all paths that used to go through
	 * grandparent now go through parent, which is black.
	 */
	rb_set_colour(pa, RB_BLACK);
	rb_set_colour(gp, RB_RED);
	if (node == pa->left)
		rb_rotate_right(root, gp);
	else
		rb_rotate_left(root, gp);
}
