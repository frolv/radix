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

#include "gen.h"

#include <stdio.h>

char *curr_partial;

#define CB_TYPE(flags) (flags & 0x3)

static void write_section(FILE *f,
                          struct rconfig_section *sec,
                          config_fn cb,
                          unsigned int flags)
{
    struct rconfig_config *conf;
    size_t i;
    int val;

    if (CB_TYPE(flags) == RCONFIG_CB_SECTION)
        cb(sec);

    fprintf(f, "\n# section %s\n", sec->name);

    for (i = 0; i < sec->num_configs; ++i) {
        conf = &sec->configs[i];
        if (CB_TYPE(flags) == RCONFIG_CB_CONFIG)
            cb(conf);

        val = conf->selection;
        fprintf(f, "CONFIG_%s=", conf->identifier);
        if (conf->type == RCONFIG_BOOL)
            fprintf(f, "%s\n", val ? "true" : "false");
        else if (conf->type == RCONFIG_INT)
            fprintf(f, "%d\n", val);
        else
            fprintf(f, "%d\n", conf->opts.options[val - 1].val);
    }
}

/*
 * generate_config:
 * Generate a partial config file from the given rconfig_file structure.
 * The output is written to config/.rconfig.${config->name}.
 *
 * `callback` is a function that gets called on each rconfig_config struct
 * in the file to get the desired value for that setting.
 */
void generate_config(struct rconfig_file *file,
                     config_fn callback,
                     unsigned int flags)
{
    FILE *f;
    size_t i;
    char path[256];

    snprintf(path, 256, CONFIG_DIR "/.rconfig.%s", file->name);
    f = fopen(path, "w");
    if (!f)
        return;

    curr_partial = path;

    if (CB_TYPE(flags) == RCONFIG_CB_FILE)
        callback(file);

    fprintf(f, "#\n");
    fprintf(f, "# rconfig %s\n", file->name);
    fprintf(f, "# %s\n", file->path);
    fprintf(f, "#\n");

    for (i = 0; i < file->num_sections; ++i)
        write_section(f, &file->sections[i], callback, flags);

    fclose(f);
    curr_partial = NULL;
}

/*
 * config_default:
 * generate_config callback which uses the default config value.
 */
void config_default(void *config)
{
    struct rconfig_config *conf = config;
    conf->selection = conf->default_val;
}
