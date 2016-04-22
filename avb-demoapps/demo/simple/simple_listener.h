/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#ifndef __SIMPLE_LISTENER_H__
#define __SIMPLE_LISTENER_H__

#include <stdint.h>
#include <arpa/inet.h>
#include <stats.h>
#include "packet.h"
#include "eavb_device.h"
#include "avtp.h"

struct app_config {
	char               *devname;
	int                entrynum;
	uint8_t            SRclassID;
	uint8_t            SRpriority;
	uint8_t            SRrank;
	uint8_t            SRvid;
	uint8_t            StreamID[AVTP_STREAMID_SIZE];
	uint64_t           framenums;
	int                fd;
	int                msrp;
	int                waitmode;
	struct app_stats   stats;
	struct eavb_device *device;
};

#endif /* __SIMPLE_LISTENER_H__ */

