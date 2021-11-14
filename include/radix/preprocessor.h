/*
 * include/radix/preprocessor.h
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

#ifndef RADIX_PREPROCESSOR_H
#define RADIX_PREPROCESSOR_H

// Returns the number of arguments in a preprocessor __VA_ARGS__ list.
// https://stackoverflow.com/questions/11317474/macro-to-count-number-of-arguments
#define ARG_COUNT(...)                                   \
	__ARG_COUNT_EVAL0(                                   \
		__HAS_COMMA(__VA_ARGS__),                \
		__HAS_COMMA(__ARG_COMMA __VA_ARGS__ ()), \
		__ARG_COUNT(__VA_ARGS__, __ARG_INDEX()))

#define __ARG_COUNT(...) __LAST_ARG(__VA_ARGS__)
#define __ARG_COUNT_EVAL0(a, b, N) __ARG_COUNT_EVAL1(a, b, N)
#define __ARG_COUNT_EVAL1(a, b, N) __ARG_COUNT_COMMA_##a##b(N)
#define __ARG_COUNT_COMMA_01(N) 0
#define __ARG_COUNT_COMMA_00(N) 1
#define __ARG_COUNT_COMMA_11(N) N


#define __LAST_ARG( \
	 _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10, \
	_11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
	_21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
	_31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
	_41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
	_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
	_61, _62, _63,   N, ...) N

#define __ARG_INDEX()                           \
	63, 62, 61, 60,                         \
	59, 58, 57, 56, 55, 54, 53, 52, 51, 50, \
	49, 48, 47, 46, 45, 44, 43, 42, 41, 40, \
	39, 38, 37, 36, 35, 34, 33, 32, 31, 30, \
	29, 28, 27, 26, 25, 24, 23, 22, 21, 20, \
	19, 18, 17, 16, 15, 14, 13, 12, 11, 10, \
	 9,  8,  7,  6,  5,  4,  3,  2,  1,  0

#define __COMMA_SEQUENCE()               \
	1, 1, 1, 1,                   \
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
	1, 1, 1, 1, 1, 1, 1, 1, 0, 0

#define __ARG_COMMA(...) ,

#define __HAS_COMMA(...) __ARG_COUNT(__VA_ARGS__, __COMMA_SEQUENCE())

#endif  // RADIX_PREPROCESSOR_H
