/*
 * include/radix/spinlock.h
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

#ifndef RADIX_SPINLOCK_H
#define RADIX_SPINLOCK_H

#include <radix/atomic.h>
#include <radix/compiler.h>
#include <radix/irq.h>

typedef int spinlock_t;

#define SPINLOCK_INIT 0

static __always_inline void __spinlock_acquire(spinlock_t *lock)
{
	while (atomic_swap(lock, 1) != 0)
		;
}

static __always_inline void __spinlock_release(spinlock_t *lock)
{
	atomic_write(lock, 0);
}

static __always_inline void spin_init(spinlock_t *lock)
{
	atomic_write(lock, 0);
}

static __always_inline void spin_lock(spinlock_t *lock)
{
	__spinlock_acquire(lock);
}

static __always_inline void spin_unlock(spinlock_t *lock)
{
	__spinlock_release(lock);
}

static __always_inline void spin_lock_irq(spinlock_t *lock)
{
	irq_disable();
	__spinlock_acquire(lock);
}

static __always_inline void spin_unlock_irq(spinlock_t *lock)
{
	__spinlock_release(lock);
	irq_enable();
}

#endif /* RADIX_SPINLOCK_H */
