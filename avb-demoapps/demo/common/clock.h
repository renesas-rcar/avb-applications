/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <stdint.h>
#include <time.h>
#include <linux/ptp_clock.h>

#ifndef CLOCK_INVALID
#define CLOCK_INVALID -1
#endif

#define CLOCKFD 3

static inline clockid_t get_clockid(int fd)
{
	return (~(clockid_t) (fd) << 3) | CLOCKFD;
}

static inline int get_clockfd(clockid_t clk_id)
{
	return (int)~((clk_id & ~((clockid_t)CLOCKFD)) >> 3);
}

clockid_t clock_parse(char *name);
extern uint64_t clock_getcount(clockid_t clk_id);

#endif /* __CLOCK_H_ */
