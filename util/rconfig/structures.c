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

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "rconfig.h"

void prepare_sections(struct rconfig_file *config)
{
	config->alloc_size = 8;
	config->sections = malloc(config->alloc_size * sizeof *config->sections);
}

/*
 * add_section:
 * Add a new section to the rconfig file `confing`.
 */
void add_section(struct rconfig_file *config, char *name)
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

	s->alloc_size = 8;
	s->num_configs = 0;
	s->configs = malloc(s->alloc_size * sizeof *s->configs);
}

/*
 * add_config:
 * Add a new config with name `identifier` to the section `section`.
 */
void add_config(struct rconfig_section *section, char *identifier)
{
	struct rconfig_config *conf;

	if (section->num_configs == section->alloc_size) {
		section->alloc_size *= 2;
		section->configs = realloc(section->configs,
		                           section->alloc_size
		                           * sizeof *section->configs);
	}

	conf = &section->configs[section->num_configs++];

	strncpy(conf->identifier, identifier, sizeof conf->identifier);
	conf->desc[0] = '\0';
	conf->type = RCONFIG_UNKNOWN;
	conf->default_val = 0;
	conf->default_set = 0;
}

void add_option(struct rconfig_config *conf, int val, char *desc)
{
	struct rconfig_option *opts = conf->opts.options;

	if (conf->opts.num_options == conf->opts.alloc_size) {
		conf->opts.alloc_size *= 2;
		opts = realloc(opts, conf->opts.alloc_size * sizeof *opts);
	}

	opts[conf->opts.num_options].val = val;
	opts[conf->opts.num_options].desc = desc;
	conf->opts.num_options++;
}

void set_config_type(struct rconfig_config *conf, int type)
{
	size_t sz;

	conf->type = type;
	switch (conf->type) {
	case RCONFIG_INT:
		conf->lim.min = INT_MIN;
		conf->lim.max = INT_MAX;
		break;
	case RCONFIG_OPTIONS:
		conf->opts.alloc_size = 8;
		conf->opts.num_options = 0;
		sz = conf->opts.alloc_size * sizeof *conf->opts.options;
		conf->opts.options = malloc(sz);
		break;
	default:
		break;
	}
}

void free_rconfig(struct rconfig_file *config)
{
	size_t i;

	for (i = 0; i < config->num_sections; ++i) {
		free(config->sections[i].name);
		free(config->sections[i].configs);
	}

	free(config->sections);
	free(config->name);
}
