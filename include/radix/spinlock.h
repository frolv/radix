/*
 * include/radix/spinlock.h
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

#ifndef RADIX_SPINLOCK_H
#define RADIX_SPINLOCK_H

#include <radix/atomic.h>
#include <radix/compiler.h>
#include <radix/irqstate.h>

// Define an opaque struct type to prevent accidental access from outside of the
// spinlock API.
typedef struct {
    unsigned long locked;
} spinlock_t;

// clang-format off
#define SPINLOCK_INIT { 0 }
// clang-format on

static __always_inline int __spinlock_try_acquire(spinlock_t *lock)
{
    return atomic_swap(&lock->locked, 1) == 0;
}

static __always_inline void __spinlock_acquire(spinlock_t *lock)
{
    while (!__spinlock_try_acquire(lock))
        ;
}

static __always_inline void __spinlock_release(spinlock_t *lock)
{
    atomic_write(&lock->locked, 0);
}

static __always_inline void spin_init(spinlock_t *lock)
{
    atomic_write(&lock->locked, 0);
}

static __always_inline void spin_lock(spinlock_t *lock)
{
    __spinlock_acquire(lock);
}

static __always_inline int spin_try_lock(spinlock_t *lock)
{
    return __spinlock_try_acquire(lock);
}

static __always_inline void spin_unlock(spinlock_t *lock)
{
    __spinlock_release(lock);
}

// Attempts to acquire a spinlock with interrupts disabled. If the acquisition
// fails, restores the original interrupt state.
static __always_inline int spin_try_lock_irq(spinlock_t *lock,
                                             unsigned long *irqstate)
{
    irq_save(*irqstate);
    if (__spinlock_try_acquire(lock)) {
        return 1;
    }
    irq_restore(*irqstate);
    return 0;
}

static __always_inline void spin_lock_irq(spinlock_t *lock,
                                          unsigned long *irqstate)
{
    irq_save(*irqstate);
    __spinlock_acquire(lock);
}

static __always_inline void spin_unlock_irq(spinlock_t *lock,
                                            unsigned long irqstate)
{
    __spinlock_release(lock);
    irq_restore(irqstate);
}

#endif  // RADIX_SPINLOCK_H
