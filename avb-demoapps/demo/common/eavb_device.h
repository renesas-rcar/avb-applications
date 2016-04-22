/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#ifndef __EAVB_DEVICE_H__
#define __EAVB_DEVICE_H__

#include "avtp.h"

#include <stdint.h>
#include <linux/if_ether.h>

struct eavb_device {
	int       fd;
	void      *framebuf;
	void      *entrybuf;
	void      *entryworkbuf;

	uint8_t   dest_addr[ETH_ALEN]; /* TODO remove */
	uint8_t   StreamID[AVTP_STREAMID_SIZE]; /* TODO remove */

	/* for entry control */
	int       entrynum;
	int       wp;
	int       rp;
	int       remain;
	int       filled;
	int       p;

	int (*get_separation_filter)(struct eavb_device *dev,
					char streamid[AVTP_STREAMID_SIZE]);
	int (*take_entry)(struct eavb_device *dev, int count);
	int (*push_entry)(struct eavb_device *dev, int count);
};

struct eavb_device *eavb_device_new(char *name, int entrynum, mode_t mode);
void eavb_device_free(struct eavb_device *dev);

#endif /* __EAVB_DEVICE_H__ */
