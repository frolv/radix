/*
 * util/rconfig/interactive.c
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
#include <stdlib.h>
#include <string.h>

#include "interactive.h"
#include "rconfig.h"

static int interactive_bool(struct rconfig_config *conf)
{
	char buf[256];

	printf("%s? (y/n) [%c] ", conf->desc, conf->default_val ? 'y' : 'n');

	if (!fgets(buf, sizeof buf, stdin)) {
		putchar('\n');
		return conf->default_val;
	} else if (buf[0] == '\n') {
		return conf->default_val;
	}

	while (strcmp(buf, "y\n") != 0 && strcmp(buf, "n\n") != 0) {
		printf("invalid input, type `y' or `n': ");
		if (!fgets(buf, sizeof buf, stdin))
			putchar('\n');
	}

	return buf[0] == 'y';
}

static int valid_number(const char *buf, int *num, int min, int max)
{
	int neg = 0;


	if (*buf == '-') {
		++buf;
		neg = 1;
	} else if (*buf == '\n') {
		printf("no number entered, try again: ");
		return 0;
	}

	*num = 0;
	for (; *buf && *buf != '\n'; ++buf) {
		if (*buf < '0' || *buf > '9') {
			printf("invalid number, try again: ");
			return 0;
		}

		*num *= 10;
		*num += *buf - '0';
	}

	*num = neg ? -(*num) : *num;

	if (*num < min || *num > max) {
		printf("number out of range, try again: ");
		return 0;
	}

	return 1;
}

static int interactive_int(struct rconfig_config *conf)
{
	char buf[256];
	int num;

	printf("%s (%d-%d) [%d] ", conf->desc, conf->lim.min,
	       conf->lim.max, conf->default_val);

	if (!fgets(buf, sizeof buf, stdin)) {
		putchar('\n');
		return conf->default_val;
	} else if (buf[0] == '\n') {
		return conf->default_val;
	}

	while (!valid_number(buf, &num, conf->lim.min, conf->lim.max)) {
		if (!fgets(buf, sizeof buf, stdin))
			putchar('\n');
	}

	return num;
}

static int interactive_options(struct rconfig_config *conf)
{
	char buf[256];
	int choice;
	size_t i;

	printf("%s [%d]\n", conf->desc, conf->default_val);
	for (i = 0; i < conf->opts.num_options; ++i)
		printf("(%lu) %s\n", i + 1, conf->opts.options[i].desc);

	if (!fgets(buf, sizeof buf, stdin)) {
		putchar('\n');
		return conf->default_val;
	} else if (buf[0] == '\n') {
		return conf->default_val;
	}

	while (!valid_number(buf, &choice, 1, conf->opts.num_options)) {
		if (!fgets(buf, sizeof buf, stdin))
			putchar('\n');
	}

	return choice;
}

#define CURR_BUFSIZE 256

static char current_file[CURR_BUFSIZE];
static char current_section[CURR_BUFSIZE];

/*
 * config_interactive:
 * Interactive rconfig callback function which prompts the user for input.
 */
int config_interactive(struct rconfig_config *conf)
{
	int ret, n;

	if (strcmp(conf->section->name, current_section) != 0) {
		if (strcmp(conf->section->file->name, current_file) != 0) {
			strncpy(current_file, conf->section->file->name,
			        CURR_BUFSIZE);
			putchar('\n');
			n = printf("%s Configuration\n", current_file);
			while (--n)
				putchar('=');
			putchar('\n');
		}
		strncpy(current_section, conf->section->name, CURR_BUFSIZE);
		putchar('\n');
		n = printf("%s\n", current_section);
		while (--n)
			putchar('-');
		putchar('\n');
	}

	switch (conf->type) {
	case RCONFIG_BOOL:
		ret = interactive_bool(conf);
		break;
	case RCONFIG_INT:
		ret = interactive_int(conf);
		break;
	case RCONFIG_OPTIONS:
		ret = interactive_options(conf);
		break;
	default:
		exit(1);
	}

	return ret;
}
