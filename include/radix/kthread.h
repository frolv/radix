/*
 * include/radix/kthread.h
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

#ifndef RADIX_KTHREAD_H
#define RADIX_KTHREAD_H

#include <radix/task.h>

#define KTHREAD_NAME_LEN 0x40

struct task *kthread_create(
    void (*func)(void *), void *arg, int page_order, char *name, ...);

struct task *kthread_run(
    void (*func)(void *), void *arg, int page_order, char *name, ...);

void kthread_start(struct task *thread);
void kthread_stop(struct task *thread);

__noreturn void kthread_exit(void);

#endif /* RADIX_KTHREAD_H */
