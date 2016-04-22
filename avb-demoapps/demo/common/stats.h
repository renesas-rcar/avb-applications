/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#ifndef __STATS_H__
#define __STATS_H__

#include <stdint.h>
#include <stdbool.h>

struct app_stats {
	bool            start;
	struct timespec stime;
	uint64_t        bytes;
	uint64_t        packets;
	uint64_t        dropped; /* TODO */
	uint64_t        errors;  /* TODO */
};

extern void stats_process(struct app_stats *stats, int length);
extern void stats_report(struct app_stats *stats, char *buf, int buflen);

#endif /* __STATS_H__ */

