/*
 * util/rconfig/structures.c
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

#include <stdlib.h>

#include "rconfig.h"

void prepare_sections(struct rconfig_file *config)
{
	config->alloc_size = 8;
	config->sections = malloc(config->alloc_size * sizeof *config->sections);
}

void free_rconfig(struct rconfig_file *config)
{
	free(config->sections);
	free(config->name);
}
