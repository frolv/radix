/*
 * util/rconfig/rconfig.h
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

#ifndef RCONFIG_H
#define RCONFIG_H

#include <stddef.h>

#define PROGRAM_NAME "rconfig"

struct rconfig_file {
	char                    *name;
	const char              *path;
	size_t                  alloc_size;
	size_t                  num_sections;
	struct rconfig_section  *sections;
};

struct rconfig_section {
	char                    *name;
	size_t                  alloc_size;
	size_t                  num_settings;
	struct rconfig_setting  *settings;
};

enum rconfig_setting_type {
	RCONFIG_BOOL,
	RCONFIG_INT,
	RCONFIG_OPTIONS
};

struct rconfig_setting_int {
	int min;
	int max;
};

struct rconfig_setting_options {
	int num_options;
	struct {
		int val;
		const char *desc;
	} *options;
};

struct rconfig_setting {
	char name[32];
	char desc[64];
	int type;
	int default_val;
	union {
		struct rconfig_setting_int data_int;
		struct rconfig_setting_options data_options;
	} data;
};

void prepare_sections(struct rconfig_file *config);
void free_rconfig(struct rconfig_file *config);

#endif /* RCONFIG_H */
