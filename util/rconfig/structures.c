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

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "lint.h"
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

void set_config_desc(struct rconfig_config *conf, char *desc)
{
	char *s;

	strncpy(conf->desc, desc + 1, 64);
	if ((s = strchr(conf->desc, '"')))
		*s = '\0';
}

static inline void set_default_val(struct rconfig_config *conf)
{
	switch (conf->type) {
	case RCONFIG_INT:
		conf->default_val = conf->lim.min;
		break;
	case RCONFIG_BOOL:
	case RCONFIG_OPTIONS:
		conf->default_val = 0;
		break;
	}
	conf->default_set = 1;
}

/*
 * verify_config:
 * Check whether `conf` is properly defined and logically consistent.
 * Return 0 if all is well, 1 if fatal error or 2 if recoverable error.
 */
int verify_config(struct rconfig_file *file, struct rconfig_config *conf)
{
	int status;
	char *s;

	status = 0;
	for (s = conf->identifier; *s && (isupper(*s) || *s == '_'); ++s)
		;
	if (*s) {
		if (is_linting)
			error("config identifiers must be ALL_CAPS\n");
		status = 1;
	}

	if (conf->type == RCONFIG_UNKNOWN) {
		if (is_linting)
			error("no type set\n");
		return 1;
	}

	if (conf->type == RCONFIG_INT) {
		if (conf->lim.min > conf->lim.max) {
			if (is_linting)
				error("range min is greater than max\n");
			status = 1;
		} else if (conf->default_set &&
		           (conf->default_val < conf->lim.min ||
		            conf->default_val > conf->lim.max)) {
			if (is_linting)
				error("default value is outside of range\n");
			status = 1;
		}
	} else if (conf->type == RCONFIG_OPTIONS) {
		if (!conf->opts.num_options) {
			if (is_linting)
				error("no options provided\n");
			status = 1;
		} else if (conf->default_set &&
		           ((unsigned)conf->default_val > conf->opts.num_options
		            || conf->default_val < 1)) {
			if (is_linting)
				error("invalid default option\n");
			status = 1;
		}
	}

	if (!conf->default_set) {
		set_default_val(conf);
		if (is_linting) {
			warn("no default value set (assuming ");
			if (conf->type == RCONFIG_BOOL)
				fprintf(stderr, "false)\n");
			else
				fprintf(stderr, "%d)\n", conf->default_val);
		}
		status = 2;
	}

	if (is_linting && status > 0)
		info("for config `\x1B[1;35m%s\x1B[0;37m' in file %s\n\n",
		     conf->identifier, file->path);

	return status;
}

static void free_options(struct rconfig_config *conf)
{
	size_t i;

	for (i = 0; i < conf->opts.num_options; ++i)
		free(conf->opts.options[i].desc);

	free(conf->opts.options);
}

void free_rconfig(struct rconfig_file *config)
{
	struct rconfig_section *s;
	size_t i, j;

	for (i = 0; i < config->num_sections; ++i) {
		s = &config->sections[i];
		for (j = 0; j < s->num_configs; ++j) {
			if (s->configs[j].type == RCONFIG_OPTIONS)
				free_options(&s->configs[j]);
		}
		free(s->name);
		free(s->configs);
	}

	free(config->name);
	free(config->sections);
}
