/*
 * include/radix/error.h
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

#ifndef RADIX_ERROR_H
#define RADIX_ERROR_H

#include <errno.h>

/*
 * The last page of virtual addresses maps to the page directory,
 * and therefore will never be returned by an allocation function.
 * It is therefore possible to return errors from a function that
 * returns a pointer through a "pointer" containing the negative
 * of one of the standard error values - a large unsigned number.
 */
#define ERR_PTR(err) ((void *)(-(err)))
#define IS_ERR(ptr)  ((unsigned long)ptr >= (unsigned long)(-ERRNO_MAX))
#define ERR_VAL(ptr) (-((unsigned long)(ptr)))

#endif /* RADIX_ERROR_H */
