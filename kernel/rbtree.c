/*
 * kernel/rbtree.c
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

#include <radix/kernel.h>
#include <radix/rbtree.h>

#define RB_BLACK 0
#define RB_RED   1

#define rb_color(node) ((node)->__parent & 1)
#define rb_set_color(node, color) \
    ((node)->__parent = (__rb_parent_addr(node)) | (color))
#define rb_set_parent(node, parent) \
    ((node)->__parent = ((unsigned long)parent) | rb_color(node))

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

    /* case 1: node is root. Set its parent to NULL and color black. */
    if (node == root->root_node) {
        node->__parent = 0;
        return;
    }

    rb_set_color(node, RB_RED);
    pa = rb_parent(node);

    /* case 2: node's parent is black. The tree is valid. */
    if (rb_color(pa) == RB_BLACK)
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
    if (un && rb_color(un) == RB_RED) {
        rb_set_color(pa, RB_BLACK);
        rb_set_color(un, RB_BLACK);
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
     * Rotate around parent around grandparent and swap their colors.
     * This satisfies property 4, as both children of grandparent will be
     * black, and property 5, as all paths that used to go through
     * grandparent now go through parent, which is black.
     */
    rb_set_color(pa, RB_BLACK);
    rb_set_color(gp, RB_RED);
    if (node == pa->left)
        rb_rotate_right(root, gp);
    else
        rb_rotate_left(root, gp);
}

/*
 * rb_replace_deleted:
 * Find the predecessor or successor of `node` if it exists,
 * swap `node` with it, and return node.
 */
static struct rb_node *rb_replace_deleted(struct rb_root *root,
                                          struct rb_node *node)
{
    struct rb_node **npos, **rpos, *npa, *rpa, *rep;

    npa = rb_parent(node);
    if (!npa)
        npos = &root->root_node;
    else
        npos = (node == npa->left) ? &npa->left : &npa->right;

    if (node->left) {
        /*
         * The predecessor of `node` is the rightmost
         * node in its left subtree.
         */
        rep = node->left;
        while (rep->right)
            rep = rep->right;
    } else if (node->right) {
        /*
         * The successor of `node` is the leftmost
         * node in its right subtree.
         */
        rep = node->right;
        while (rep->left)
            rep = rep->left;
    } else {
        return node;
    }

    rpa = rb_parent(rep);
    rpos = (rep == rpa->left) ? &rpa->left : &rpa->right;

    /* swap the positions of `rep` and `node` in the tree */
    *rpos = node;
    *npos = rep;

    /* swap and update rep's and node's children */
    swap(rep->__parent, node->__parent);
    swap(rep->left, node->left);
    if (rep->left)
        rb_set_parent(rep->left, rep);
    if (node->left)
        rb_set_parent(node->left, node);

    swap(rep->right, node->right);
    if (rep->right)
        rb_set_parent(rep->right, rep);
    if (node->right)
        rb_set_parent(node->right, node);

    return node;
}

/*
 * rb_remove:
 * Remove `node` from the tree rooted at `root`.
 * Precondition: `node` has at most one child.
 */
static void rb_remove(struct rb_root *root, struct rb_node *node)
{
    struct rb_node *pa, *child, *sl, **nptr;

    pa = rb_parent(node);
    child = node->left ? node->left : node->right;

    if (!pa)
        nptr = &root->root_node;
    else if (node == pa->left)
        nptr = &pa->left;
    else
        nptr = &pa->right;

    /* Replace `node` with its child (which might be NULL) */
    *nptr = child;

    /*
     * If `node` is red, then it cannot have any children and therefore
     * can be replaced with a black NULL leaf without violating any
     * tree properties.
     */
    if (rb_color(node) == RB_RED)
        return;

    /*
     * If `node` is black and its child is red, then its child can
     * be repainted black to preserve all tree properties.
     *
     * Note that checking for the existence of `child` is sufficent:
     * if `node` has a child, it must be red, otherwise property 5
     * would be violated.
     */
    if (child) {
        rb_set_color(child, RB_BLACK);
        rb_set_parent(child, pa);
        return;
    }

    node = NULL;

begin_rebalance:
    /*
     * case 1: `node` was the root.
     * The tree is now empty.
     */
    if (!pa)
        return;

    sl = (node == pa->left) ? pa->right : pa->left;

    /*
     * case 2: sibling of `node` is red.
     * This means that the parent must be black. Swap colors of parent
     * and sibling and rotate them to set up case 4, 5, or 6.
     */
    if (rb_color(sl) == RB_RED) {
        rb_set_color(pa, RB_RED);
        rb_set_color(sl, RB_BLACK);
        if (sl == pa->left)
            rb_rotate_right(root, pa);
        else
            rb_rotate_left(root, pa);

        sl = (node == pa->left) ? pa->right : pa->left;
    }

    /*
     * case 3: sibling and its children are black, and so is parent.
     * Sibling's side of the tree has one more black node than node's,
     * which is corrected by changing sibling to red. However, this results
     * in all paths passing through parent having one fewer black node than
     * before, so a rebalance is performed on parent.
     *
     * case 4: sibling and its children are black but parent is red.
     * The colors of sibling and parent are swapped, adding one black node
     * to paths going through `node`, without changing the number of black
     * nodes in paths going through sibling, thus balancing the tree.
     */
    if ((!sl->left || rb_color(sl->left) == RB_BLACK) &&
        (!sl->right || rb_color(sl->right) == RB_BLACK)) {
        if (rb_color(pa) == RB_BLACK) {
            rb_set_color(sl, RB_RED);
            node = pa;
            pa = rb_parent(node);
            goto begin_rebalance;
        } else {
            rb_set_color(sl, RB_RED);
            rb_set_color(pa, RB_BLACK);
            return;
        }
    }

    /*
     * case 5: sibling's red child is on the opposite side of sibling
     *         than sibling is of parent.
     * Rotate around sibling and change its color, placing node's new
     * sibling and its red child on the same side, setting up case 6.
     */
    if (sl == pa->right && sl->left && rb_color(sl->left) == RB_RED) {
        rb_set_color(sl, RB_RED);
        rb_set_color(sl->left, RB_BLACK);
        rb_rotate_right(root, sl);
        sl = pa->right;
    } else if (sl == pa->left && sl->right && rb_color(sl->right) == RB_RED) {
        rb_set_color(sl, RB_RED);
        rb_set_color(sl->right, RB_BLACK);
        rb_rotate_left(root, sl);
        sl = pa->left;
    }

    /*
     * case 6: sibling's red child is on the same side of sibling
     *         as sibling is of parent.
     * The colors of parent and sibling are exchanged, sibling's red child
     * is made black, and a rotation is performed around parent. This makes
     * sibling the new root of the subtree, with the same color as the old
     * root, and adds one extra black node to all paths through `node`.
     * Done.
     */
    rb_set_color(sl, rb_color(pa));
    rb_set_color(pa, RB_BLACK);
    if (sl == pa->right) {
        rb_set_color(sl->right, RB_BLACK);
        rb_rotate_left(root, pa);
    } else {
        rb_set_color(sl->left, RB_BLACK);
        rb_rotate_right(root, pa);
    }
}

/*
 * rb_delete:
 * Delete `node` from the tree rooted at `root`.
 */
void rb_delete(struct rb_root *root, struct rb_node *node)
{
    struct rb_node *n;

    /* `node` is not part of a tree */
    if (unlikely(!node || rb_parent(node) == node))
        return;

    n = rb_replace_deleted(root, node);
    rb_remove(root, n);
    rb_init(n);
}

/*
 * rb_replace:
 * Replace node `old` with `new` in the tree rooted at `root`.
 * If `new` does not fit in the same position as `old`, this breaks the tree.
 */
void rb_replace(struct rb_root *root, struct rb_node *old, struct rb_node *new)
{
    struct rb_node *pa, **pos;

    pa = rb_parent(old);
    if (!pa)
        pos = &root->root_node;
    else
        pos = (old == pa->left) ? &pa->left : &pa->right;

    *pos = new;
    new->__parent = old->__parent;
    new->left = old->left;
    new->right = old->right;

    rb_init(old);
}
