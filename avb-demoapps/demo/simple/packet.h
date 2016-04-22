/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#ifndef __PACKET_H__
#define __PACKET_H__

#include <stdint.h>
#include <linux/if_ether.h>
#include "avtp.h"

/* ether header(6+6+2=14), Q-Tag(4) */
#define ETHOVERHEAD (14 + 4)
/* preamble(8), ETHOVERHEAD, CRC(4) */
#define ETHOVERHEAD_REAL (8 + ETHOVERHEAD + 4)
/* ETHOVERHEAD_REAL, IFG(12) */
#define ETHOVERHEAD_IFG (ETHOVERHEAD_REAL + 12)

#define ETHFRAMEMTU_MIN   (46)
#define ETHFRAMEMTU_MAX   (1500)
#define ETHFRAMELEN_MIN   (ETHFRAMEMTU_MIN + ETHOVERHEAD)
#define ETHFRAMELEN_MAX   (ETHFRAMEMTU_MAX + ETHOVERHEAD)

struct avtp_simple_param {
	char dest_addr[ETH_ALEN];
	char source_addr[ETH_ALEN];
	int payload_size;
	int uniqueid;
	int SRpriority;
	int SRvid;
};

extern int avtp_simple_header_build(void *dst, struct avtp_simple_param *param);

#endif /* __PACKET_H__ */
