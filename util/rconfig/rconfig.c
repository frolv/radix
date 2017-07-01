/*
 * util/rconfig/rconfig.c
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

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define PROGRAM_NAME "rconfig"

const char *src_dirs[] = { "kernel", "drivers", "lib", NULL };

#define NUM_SRC_DIRS   (sizeof src_dirs / sizeof (src_dirs[0]))
#define ARCH_DIR_INDEX (NUM_SRC_DIRS - 1)
#define ARCHDIR_BUFSIZE 32

static int verify_src_dirs(const char *prog)
{
	struct stat sb;
	size_t i;

	for (i = 0; i < NUM_SRC_DIRS; ++i) {
		if (stat(src_dirs[i], &sb) == 0) {
			if (S_ISDIR(sb.st_mode))
				continue;

			fprintf(stderr, "%s: %s\n",
				src_dirs[i],
				strerror(ENOTDIR));
			goto err_wrongdir;
		}

		perror(src_dirs[i]);
		if (i == ARCH_DIR_INDEX) {
			fprintf(stderr, "%s: invalid or unsupported architecture\n",
				prog);
			return 1;
		}

	err_wrongdir:
		fprintf(stderr, "%s: are you in the radix root directory?\n",
			prog);
		return 1;
	}

	return 0;
}

void usage(FILE *f, const char *prog)
{
	fprintf(f, "usage: %s --arch=ARCH [-d|-l] [FILE]...\n", prog);
	fprintf(f, "Configure a radix kernel\n");
	fprintf(f, "\n");
	fprintf(f, "If FILE is provided, only process given rconfig files.\n");
	fprintf(f, "Otherwise, recursively process every rconfig file in\n");
	fprintf(f, "the radix kernel tree.\n");
	fprintf(f, "\n");
	fprintf(f, "    -a, --arch=ARCH\n");
	fprintf(f, "        use ARCH as target architecture\n");
	fprintf(f, "    -d, --default\n");
	fprintf(f, "        use default values from rconfig files\n");
	fprintf(f, "    -h, --help\n");
	fprintf(f, "        print this help text and exit\n");
	fprintf(f, "    -l, --lint\n");
	fprintf(f, "        verify rconfig file syntax and structure\n");
}

static struct option long_opts[] = {
	{ "arch",       required_argument, NULL, 'a' },
	{ "default",    no_argument,       NULL, 'd' },
	{ "help",       no_argument,       NULL, 'h' },
	{ "lint",       no_argument,       NULL, 'l' },
	{ 0, 0, 0, 0 }
};

int main(int argc, char **argv)
{
	char arch_dir[ARCHDIR_BUFSIZE];
	int c, err;

	while ((c = getopt_long(argc, argv, "a:dhl", long_opts, NULL)) != EOF) {
		switch (c) {
		case 'a':
			snprintf(arch_dir, ARCHDIR_BUFSIZE, "arch/%s", optarg);
			src_dirs[ARCH_DIR_INDEX] = arch_dir;
			break;
		case 'h':
			usage(stdout, PROGRAM_NAME);
			return 0;
		default:
			usage(stderr, argv[0]);
			return 1;
		}
	}

	if (!src_dirs[ARCH_DIR_INDEX]) {
		fprintf(stderr, "%s: must provide target architecture\n",
			argv[0]);
		return 1;
	}

	if ((err = verify_src_dirs(argv[0])) != 0)
		return 1;

	return 0;
}
