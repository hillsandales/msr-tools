/* ----------------------------------------------------------------------- *
 *
 *   Copyright 2000 Transmeta Corporation - All Rights Reserved
 *   Copyright 2004-2008 H. Peter Anvin - All Rights Reserved
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *   Boston MA 02110-1301, USA; either version 2 of the License, or
 *   (at your option) any later version; incorporated herein by reference.
 *
 * ----------------------------------------------------------------------- */

/*
 * rmwmsr.c
 *
 * Utility to read, modify and write to an MSR.
 */

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>

#include "version.h"

static const struct option long_options[] = {
	{"help",	0, 0, 'h'},
	{"version",	0, 0, 'V'},
	{"all",		0, 0, 'a'},
	{"processor",	1, 0, 'p'},
	{"cpu",		1, 0, 'p'},
	{"mask",	1, 0, 'm'},
	{0, 0, 0, 0}
};
static const char short_options[] = "hVap:m:";
static int doing_for_all = 0;
static bool rdmsr_required = false;

const char *program;

void usage(void)
{
	fprintf(stderr, "Usage: %s [options] regno value...\n"
		"wr_data = (rd_data & ~mask) | value\n"
		"  --help         -h  Print this help\n"
		"  --version      -V  Print current version\n"
		"  --all          -a  all processors\n"
		"  --processor #  -p  Select processor number (default 0)\n"
		"  --mask <hex>   -m  AND mask [default: 0xffff_ffff_ffff_ffff]\n",
		program);
}

void rmwmsr_on_cpu(uint32_t reg, int cpu, unsigned long mask, int valcnt, char *regvals[]);

/* filter out ".", "..", "microcode" in /dev/cpu */
int dir_filter(const struct dirent *dirp)
{
	if (isdigit(dirp->d_name[0]))
		return 1;
	else
		return 0;
}

void rmwmsr_on_all_cpus(uint32_t reg, unsigned long mask, int valcnt, char *regvals[])
{
	struct dirent **namelist;
	int dir_entries;

	dir_entries = scandir("/dev/cpu", &namelist, dir_filter, 0);
	while (dir_entries--) {
		rmwmsr_on_cpu(reg, atoi(namelist[dir_entries]->d_name),
				mask, valcnt, regvals);
		free(namelist[dir_entries]);
	}
	free(namelist);
}

int main(int argc, char *argv[])
{
	uint32_t reg;
	int c;
	int cpu = 0;
	unsigned long arg;
	unsigned long mask = ULONG_MAX;
	char *endarg;

	program = argv[0];

	while ((c = getopt_long(argc, argv, short_options,
				long_options, NULL)) != -1) {
		switch (c) {
		case 'h':
			usage();
			exit(0);
		case 'V':
			fprintf(stderr, "%s: version %s\n", program,
				VERSION_STRING);
			exit(0);
		case 'a':
			cpu = -1;
			break;
		case 'p':
			arg = strtoul(optarg, &endarg, 0);
			if (*endarg || arg > 5119) {
				usage();
				exit(127);
			}
			cpu = (int)arg;
			break;
		case 'm':
			mask = strtoul(optarg, &endarg, 0);
			if (*endarg) {
				usage();
				exit(127);
			}
			break;
		default:
			usage();
			exit(127);
		}
	}

	if (mask != ULONG_MAX) {
		rdmsr_required = true;
	}

	if (optind > argc - 2) {
		/* Should have at least two arguments */
		usage();
		exit(127);
	}

	reg = strtoul(argv[optind++], NULL, 0);

	if (cpu == -1) {
		doing_for_all = 1;
		rmwmsr_on_all_cpus(reg, mask, argc - optind, &argv[optind]);
	} else {
		rmwmsr_on_cpu(reg, cpu, mask, argc - optind, &argv[optind]);
	}

	exit(0);
}

void rmwmsr_on_cpu(uint32_t reg, int cpu, unsigned long mask, int valcnt, char *regvals[])
{
	uint64_t data = 0;
	int fd;
	char msr_file_name[64];

	sprintf(msr_file_name, "/dev/cpu/%d/msr", cpu);

	// Read the MSR
	if(rdmsr_required) {
		fd = open(msr_file_name, O_RDONLY);
		if (fd < 0) {
			if (errno == ENXIO) {
				if (doing_for_all)
					return;
				fprintf(stderr, "rdmsr: No CPU %d\n", cpu);
				exit(2);
			} else if (errno == EIO) {
				fprintf(stderr, "rdmsr: CPU %d doesn't support MSRs\n",
					cpu);
				exit(3);
			} else {
				perror("rdmsr: open");
				exit(127);
			}
		}

		if (pread(fd, &data, sizeof data, reg) != sizeof data) {
			if (errno == EIO) {
				fprintf(stderr, "rdmsr: CPU %d cannot read "
					"MSR 0x%08"PRIx32"\n",
					cpu, reg);
				exit(4);
			} else {
				perror("rdmsr: pread");
				exit(127);
			}
		}

		close(fd);

		printf("CPU %d Read: %lx\n", cpu, data);
		// Apply mask
		data &= (~mask);
	}

	// Write the MSR
	fd = open(msr_file_name, O_WRONLY);
	if (fd < 0) {
		if (errno == ENXIO) {
			if (doing_for_all)
				return;
			fprintf(stderr, "wrmsr: No CPU %d\n", cpu);
			exit(2);
		} else if (errno == EIO) {
			fprintf(stderr, "wrmsr: CPU %d doesn't support MSRs\n",
				cpu);
			exit(3);
		} else {
			perror("wrmsr: open");
			exit(127);
		}
	}

	while (valcnt--) {
		data |= strtoull(*regvals++, NULL, 0);
		if(rdmsr_required) {
			printf("CPU %d Writing: %lx\n", cpu, data);
		}
		if (pwrite(fd, &data, sizeof data, reg) != sizeof data) {
			if (errno == EIO) {
				fprintf(stderr,
					"wrmsr: CPU %d cannot set MSR "
					"0x%08"PRIx32" to 0x%016"PRIx64"\n",
					cpu, reg, data);
				exit(4);
			} else {
				perror("wrmsr: pwrite");
				exit(127);
			}
		}
	}

	close(fd);

	return;
}
