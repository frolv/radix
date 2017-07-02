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

void add_section(struct rconfig_file *config, char *name,
                 struct rconfig_setting **settings)
{
	struct rconfig_section *s;

	if (config->num_sections == config->alloc_size) {
		config->alloc_size *= 2;
		config->sections = realloc(config->sections,
		                           config->alloc_size
		                           * sizeof *config->sections);
	}

	s = &config->sections[config->num_sections++];
	s->name = name;
	s->settings = settings;
}

void free_rconfig(struct rconfig_file *config)
{
	size_t i;

	for (i = 0; i < config->num_sections; ++i) {
		free(config->sections[i].name);
	}

	free(config->sections);
	free(config->name);
}
