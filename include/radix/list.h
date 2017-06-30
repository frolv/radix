/*
 * include/radix/list.h
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

#ifndef RADIX_LIST_H
#define RADIX_LIST_H

#include <radix/compiler.h>

struct list {
	struct list *next;
	struct list *prev;
};

#define LIST_INIT(name) { &(name), &(name) }

static __always_inline void list_init(struct list *list)
{
	list->next = list->prev = list;
}

/*
 * Insert elem into the list between the entries head and prev.
 * This is internal for the two functions below.
 */
static __always_inline void __insert(struct list *elem, struct list *prev,
                                     struct list *next)
{
	elem->next = next;
	elem->prev = prev;
	next->prev = elem;
	prev->next = elem;
}

/*
 * Insert elem into the list after head.
 */
static __always_inline void list_add(struct list *head, struct list *elem)
{
	__insert(elem, head, head->next);
}

/*
 * Insert elem into the list before head.
 */
static __always_inline void list_ins(struct list *head, struct list *elem)
{
	__insert(elem, head->prev, head);
}

/*
 * Delete the entry elem from the list.
 */
static __always_inline void list_del(struct list *elem)
{
	elem->prev->next = elem->next;
	elem->next->prev = elem->prev;
	elem->prev = elem->next = elem;
}

static __always_inline int list_empty(struct list *head)
{
	return head->next == head;
}

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_first_entry(head, type, member) \
	list_entry((head)->next, type, member)

#define list_last_entry(head, type, member) \
	list_entry((head)->prev, type, member)

#define list_next_entry(pos, member) \
	list_entry((&(pos)->member)->next, typeof(*pos), member)

#define list_prev_entry(pos, member) \
	list_entry((&(pos)->member)->prev, typeof(*pos), member)

#define list_for_each(pos, head) \
	for ((pos) = (head)->next; (pos) != (head); (pos) = (pos)->next)

#define list_for_each_r(pos, head) \
	for ((pos) = (head)->prev; (pos) != (head); (pos) = (pos)->prev)

#define list_for_each_safe(pos, n, head)                                \
	for ((pos) = (head)->next, (n) = (pos)->next;                   \
	     (pos) != (head);                                           \
	     (pos) = (n), (n) = (pos)->next)

#define list_for_each_safe_r(pos, n, head)                              \
	for ((pos) = (head)->prev, (n) = (pos)->prev;                   \
	     (pos) != (head);                                           \
	     (pos) = (n), (n) = (pos)->prev)

#define list_for_each_entry(pos, head, member)                          \
	for ((pos) = list_first_entry(head, typeof(*pos), member);      \
	     &(pos)->member != (head);                                  \
	     (pos) = list_next_entry(pos, member))

#define list_for_each_entry_r(pos, head, member)                        \
	for ((pos) = list_last_entry(head, typeof(*pos), member);       \
	     &(pos)->member != (head);                                  \
	     (pos) = list_prev_entry(pos, member))

#endif /* RADIX_LIST_H */
