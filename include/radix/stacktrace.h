/*
 * include/radix/stacktrace.h
 * Copyright (C) 2018 Alexei Frolov
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

#ifndef RADIX_STACKTRACE_H
#define RADIX_STACKTRACE_H

#if defined(CONFIG_DEBUG) && defined(CONFIG_DEBUG_STACKTRACE)

#include <radix/limits.h>
#include <radix/types.h>

#define DEBUG_STACKTRACE

#if CONFIG_STACKTRACE_DEPTH == 0
#define STACKTRACE_DEPTH INT_MAX
#else
#define STACKTRACE_DEPTH CONFIG_STACKTRACE_DEPTH
#endif

int stack_trace(char *buf, size_t size);

#endif /* CONFIG_DEBUG && CONFIG_DEBUG_STACKTRACE */

#endif /* RADIX_STACKTRACE_H */
