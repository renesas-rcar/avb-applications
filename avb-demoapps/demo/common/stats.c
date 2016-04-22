/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <stdbool.h>

#include "stats.h"

static inline double stats_guess_unit(double value)
{
	if (value <= 1000)
		return 1;
	else if (value <= 1000000)
		return 1000;
	else if (value <= 1000000000)
		return 1000000;
	else /* (value > 1000000000) */
		return 1000000000;
}

static inline char *stats_guess_label(double value)
{
	if (value <= 1000)
		return "";
	else if (value <= 1000000)
		return "K";
	else if (value <= 1000000000)
		return "M";
	else /* (value > 1000000000) */
		return "G";
}

/*
 * public functions
 */
void stats_process(struct app_stats *stats, int length)
{
	if (!stats->start) {
		clock_gettime(CLOCK_MONOTONIC, &stats->stime);
		stats->start = true;
	}

	stats->bytes += length;
	stats->packets++;
}

void stats_report(struct app_stats *stats, char *buf, int buflen)
{
	struct timespec etime, stime;
	uint64_t et, st;
	double bps, duration;
	double total;

	/* not started */
	if (!stats->start)
		return;

	clock_gettime(CLOCK_MONOTONIC, &etime);
	memcpy(&stime, &stats->stime, sizeof(stime));

	total = (double)stats->bytes;

	st = (stime.tv_sec * 1000000) + (stime.tv_nsec / 1000);
	et = (etime.tv_sec * 1000000) + (etime.tv_nsec / 1000);
	duration = (double)(et - st) / 1000000;
	bps = (double)(stats->bytes * 8) / duration;

	snprintf(buf, buflen, "%"PRIu64"packets %.3f%sB/%.3fs=%.3f%sbps",
		stats->packets,
		total/stats_guess_unit(total), stats_guess_label(total),
		duration,
		bps/stats_guess_unit(bps), stats_guess_label(bps));
}
