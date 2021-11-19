/*
 * util/rconfig/structures.h
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

#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <rconfig.h>

void prepare_sections(struct rconfig_file *config);
void add_section(struct rconfig_file *config, char *name);
void add_config(struct rconfig_section *section, char *identifier);
void add_option(struct rconfig_config *conf, int val, char *desc);
void set_config_type(struct rconfig_config *conf, int type);
void set_config_desc(struct rconfig_config *conf, char *desc);
int verify_config(struct rconfig_file *file, struct rconfig_config *conf);

static inline struct rconfig_config *curr_config(struct rconfig_file *file)
{
    struct rconfig_section *s;

    s = &file->sections[file->num_sections - 1];
    return &s->configs[s->num_configs - 1];
}

void free_rconfig(struct rconfig_file *config);

#endif /* STRUCTURES_H */
