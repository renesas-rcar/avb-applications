/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "clock.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#define NSEC_SCALE (1000000000)

/*
 * public functions
 */
clockid_t clock_parse(char *name)
{
	struct {
		char *name;
		clockid_t clkid;
	} clkid_table[] = {
		{ "CLOCK_REALTIME", CLOCK_REALTIME },
		{ "CLOCK_MONOTONIC", CLOCK_MONOTONIC },
		{ "CLOCK_MONOTONIC_RAW", CLOCK_MONOTONIC_RAW },
	};
	clockid_t clkid;

	/* if clock identifier */
	if (!strncmp(name, "CLOCK_", strlen("CLOCK_"))) {
		int i, len;

		for (i = 0; i < ARRAY_SIZE(clkid_table); i++) {
			len = strlen(clkid_table[i].name);
			if (!strncmp(name, clkid_table[i].name, len) && !name[len]) {
				clkid = clkid_table[i].clkid;
				break;
			}
		}

		if (i == ARRAY_SIZE(clkid_table))
			return CLOCK_INVALID;
	} else {
		/* try open as a ptp clock */
		struct ptp_clock_caps caps;
		int fd;

		fd = open(name, O_RDWR);
		if (fd < 0) {
			perror("open");
			return CLOCK_INVALID;
		}
		clkid = get_clockid(fd);
		if (CLOCK_INVALID == clkid)
			return CLOCK_INVALID;

		if (ioctl(fd, PTP_CLOCK_GETCAPS, &caps)) {
			perror("PTP_CLOCK_GETCAPS");
			return CLOCK_INVALID;
		}
	}

	return clkid;
}

uint64_t clock_getcount(clockid_t clk_id)
{
	struct timespec ts;
	uint64_t t;

	clock_gettime(clk_id, &ts);

	/* convert timespec to uint64 */
	t = ts.tv_sec;
	t *= NSEC_SCALE;
	t += ts.tv_nsec;

	return t;
}
