/*
 * include/radix/mutex.h
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

#ifndef RADIX_MUTEX_H
#define RADIX_MUTEX_H

#include <radix/list.h>
#include <radix/spinlock.h>

#define MUTEX_INIT(name) { 0, LIST_INIT((name).queue), SPINLOCK_INIT }

struct mutex {
	int             count;
	struct list     queue;
	spinlock_t      lock;
};

void mutex_init(struct mutex *m);
void mutex_lock(struct mutex *m);
void mutex_unlock(struct mutex *m);

#endif /* RADIX_MUTEX_H */
