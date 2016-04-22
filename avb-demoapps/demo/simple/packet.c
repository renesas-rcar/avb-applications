/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#include <string.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>

#include "config.h"
#include "packet.h"
#include "avtp.h"

/*
 * simple header build
 */
int avtp_simple_header_build(void *dst, struct avtp_simple_param *param)
{
	int hlen, len;
	uint8_t cfi = 0;
	uint8_t streamid[AVTP_STREAMID_SIZE];

	memset(dst, 0, ETHFRAMELEN_MAX);   /* clear MAC frame buffer */

	/* Ethernet frame header */
	set_ieee8021q_dest(dst, (uint8_t *)param->dest_addr);
	set_ieee8021q_source(dst, (uint8_t *)param->source_addr);

	/* IEEE802.1Q Q-tag */
	cfi = 0;

	set_ieee8021q_tpid(dst, ETH_P_8021Q);
	/* pcp:3bit, cfi:1bit, vid:12bit */
	set_ieee8021q_tci(dst, (param->SRpriority << 13) |
				(cfi << 12) | param->SRvid);
	set_ieee8021q_ethtype(dst, ETH_P_1722);

	hlen = AVTP_CVF_PAYLOAD_OFFSET;
	len = param->payload_size;

	/* 1722 header update + payload */
	memcpy(streamid, param->source_addr, ETH_ALEN);
	streamid[6] = (param->uniqueid & 0xff00) >> 8;
	streamid[7] = param->uniqueid & 0x00ff;

	copy_avtp_cvf_experimental_template(dst);
	set_avtp_stream_id(dst, streamid);
	set_avtp_stream_data_length(dst, len);

	return hlen + len;
}
