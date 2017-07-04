/*
 * util/rconfig/lib/lint.h
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

#ifndef LINT_H
#define LINT_H

#include <stdio.h>

#define error(msg, ...) \
	fprintf(stderr, "\x1B[1;31merror: \x1B[0;37m" msg, ##__VA_ARGS__)

#define warn(msg, ...) \
	fprintf(stderr, "\x1B[1;35mwarning: \x1B[0;37m" msg, ##__VA_ARGS__)

#define info(msg, ...) \
	fprintf(stderr, "\x1B[1;34minfo: \x1B[0;37m" msg, ##__VA_ARGS__)

#endif /* LINT_H */
