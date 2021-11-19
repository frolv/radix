/*
 * lib/rlibc/string/strrev.c
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

#include <rlibc/string.h>

char *strrev(char *s)
{
    char *start, *t;
    char c;

    start = s;
    t = s + strlen(s) - 1;

    while (s < t) {
        c = *t;
        *t-- = *s;
        *s++ = c;
    }

    return start;
}
