/*
 * util/rconfig/frontends/cli/cli.c
 * rconfig cli frontend
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

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <rconfig.h>

#include "interactive.h"

static void sig_cleanup(int sig)
{
	printf("\n");
	printf("Received SIG%s, exiting...\n", sig == SIGINT ? "INT" : "TERM");

	rconfig_cleanup_partial();
	exit(0);
}

static void usage(FILE *f, const char *prog)
{
	fprintf(f, "usage: %s --arch=ARCH [-d|-l] [-o OUTFILE] [FILE]...\n",
	        prog);
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
	fprintf(f, "    -o, --output=OUTFILE\n");
	fprintf(f, "        write output to OUTFILE\n");
}

static struct option long_opts[] = {
	{ "arch",       required_argument, NULL, 'a' },
	{ "default",    no_argument,       NULL, 'd' },
	{ "help",       no_argument,       NULL, 'h' },
	{ "lint",       no_argument,       NULL, 'l' },
	{ "output",     required_argument, NULL, 'o' },
	{ 0, 0, 0, 0 }
};

int main(int argc, char **argv)
{
	char arch_dir[32];
	char *arch;
	config_fn callback;
	struct stat sb;
	int c, err;
	const char *errdir;
	char outfile[256] = "config/config";

	callback = config_interactive;
	arch = NULL;

	while ((c = getopt_long(argc, argv, "a:dhlo:", long_opts, NULL))
	       != EOF) {
		switch (c) {
		case 'a':
			arch = optarg;
			snprintf(arch_dir, sizeof arch_dir, "arch/%s", arch);
			rconfig_set_archdir(arch_dir);
			break;
		case 'd':
			callback = config_default;
			break;
		case 'h':
			usage(stdout, PROGRAM_NAME);
			return 0;
		case 'l':
			is_linting = 1;
			break;
		case 'o':
			strncpy(outfile, optarg, sizeof outfile);
			break;
		default:
			usage(stderr, argv[0]);
			return 1;
		}
	}

	if (!arch) {
		fprintf(stderr, "%s: must provide target architecture\n",
			argv[0]);
		return 1;
	}

	if ((err = rconfig_verify_src_dirs(&errdir)) != 0) {
		switch (err) {
		case EINVAL:
			fprintf(stderr, "%s: invalid or unsupported architecture\n",
			        argv[0]);
			break;
		default:
			fprintf(stderr, "%s: %s\n", errdir, strerror(err));
			fprintf(stderr, "%s: are you in the radix root directory?\n",
			        argv[0]);
			break;
		}
		return 1;
	}

	signal(SIGINT, sig_cleanup);
	signal(SIGTERM, sig_cleanup);

	if (!is_linting && callback == config_interactive) {
		printf(PROGRAM_NAME " " PROGRAM_VERSION " interactive mode\n");
		printf("Configuring radix for target architecture %s\n", arch);
	}

	if (optind != argc) {
		for (; optind < argc; ++optind) {
			if (stat(argv[optind], &sb) != 0) {
				perror(argv[optind]);
				exit_status = 1;
			} else if (!S_ISREG(sb.st_mode)) {
				fprintf(stderr, "%s: not a regular file\n",
				        argv[optind]);
				exit_status = 1;
			} else {
				rconfig_parse_file(argv[optind], callback,
				                   RCONFIG_CB_CONFIG);
			}
		}
	} else {
		rconfig_recursive(callback, RCONFIG_CB_CONFIG);
	}

	if ((err = rconfig_concatenate(outfile)) != 0) {
		fprintf(stderr, "%s: could not concatenate partial configs\n",
		        argv[0]);
		exit_status = 1;
	} else if (!is_linting && callback == config_interactive) {
		printf("\n");
		printf("radix configuration complete\n");
		printf("Configuration written to file %s\n", outfile);
	}

	return exit_status;
}
