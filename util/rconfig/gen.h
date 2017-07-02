/*
 * util/rconfig/gen.h
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

#ifndef GEN_H
#define GEN_H

struct rconfig_file;
struct rconfig_config;

typedef int (*setting_fn)(struct rconfig_config *);

void generate_config(struct rconfig_file *config, setting_fn fn);

int config_default(struct rconfig_config *config);

#endif /* GEN_H */
