/*
 * include/rlibc/ctype.h
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

#ifndef RLIBC_CTYPE_H
#define RLIBC_CTYPE_H

int isalnum(int c);
int isalpha(int c);
int isdigit(int c);
int isspace(int c);
int islower(int c);
int isupper(int c);

int tolower(int c);
int toupper(int c);

#endif /* RLIBC_CTYPE_H */
