/*
 * util/rconfig/gen.c
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

#include <stdio.h>

#include "gen.h"
#include "rconfig.h"

static void read_section(FILE *f, struct rconfig_section *sec, setting_fn fn)
{
	struct rconfig_config *conf;
	size_t i;
	int val;

	fprintf(f, "\n# section %s\n", sec->name);

	for (i = 0; i < sec->num_configs; ++i) {
		conf = &sec->configs[i];
		val = fn(conf);

		fprintf(f, "CONFIG_%s=", conf->identifier);
		if (conf->type == RCONFIG_BOOL)
			fprintf(f, "%s\n", val ? "true" : "false");
		else if (conf->type == RCONFIG_INT)
			fprintf(f, "%d\n", val);
		else
			fprintf(f, "%d\n", conf->opts.options[val - 1].val);
	}
}

void generate_config(struct rconfig_file *config, setting_fn fn)
{
	FILE *f;
	size_t i;
	char path[256];

	snprintf(path, 256, "config/.rconfig.%s", config->name);
	f = fopen(path, "w");
	if (!f)
		return;

	fprintf(f, "#\n");
	fprintf(f, "# rconfig %s\n", config->name);
	fprintf(f, "# %s\n", config->path);
	fprintf(f, "#\n");

	for (i = 0; i < config->num_sections; ++i)
		read_section(f, &config->sections[i], fn);

	fclose(f);
}

int config_default(struct rconfig_config *config)
{
	return config->default_val;
}
