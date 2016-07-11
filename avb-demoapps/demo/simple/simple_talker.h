/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#ifndef __SIMPLE_TALKER_H__
#define __SIMPLE_TALKER_H__

#include <stdint.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include "netif_util.h"
#include "packet.h"
#include "eavb_device.h"

#define NSEC_SCALE	(1000000000)

struct app_config {
	int                fd;
	char               ifname[IFNAMSIZ];
	double             bandwidthFraction;
	int                entrynum;
	uint8_t            SRclassID;
	uint8_t            SRpriority;
	uint8_t            SRrank;
	uint8_t            SRvid;
	int                SRclassIntervalFrames;
	clockid_t          clkid;
	int                uid;
	uint8_t            StreamID[AVTP_STREAMID_SIZE];
	uint8_t            dest_addr[ETH_ALEN];
	uint8_t            source_addr[ETH_ALEN];
	uint16_t           MaxFrameSize;
	uint16_t           payload_size;
	uint16_t           MaxIntervalFrames;
	uint64_t           framenums;
	int                speed;
	int                msrp;
	int                waitmode;
	bool               use_dest_addr;
	struct eavb_device *device;
};

#endif /* __SIMPLE_TALKER_H__ */

