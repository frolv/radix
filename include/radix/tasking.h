/*
 * include/radix/tasking.h
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

#ifndef RADIX_TASKING_H
#define RADIX_TASKING_H

#include <radix/task.h>

void tasking_init(void);
struct task *task_alloc(void);
void task_free(struct task *task);

void task_exit(struct task *task, int status);

void switch_task(struct task *old, struct task *new);

#endif  // RADIX_TASKING_H
