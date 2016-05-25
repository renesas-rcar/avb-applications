/******************************************************************************

  Copyright (c) 2012, Intel Corporation
  Copyright (C) 2014-2016, Renesas Electronics Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   3. Neither the name of the Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/

#ifndef __MSRP_H__
#define __MSRP_H__

#include <stdbool.h>
#include "mrpd.h"
#include "mrpdclient.h"
#include "mrpdhelper.h"

/* Class ID definitions */
#define MSRP_SR_CLASS_A	(6)
#define MSRP_SR_CLASS_B	(5)
#define MSRP_SR_CLASS_C	(4)

/* default values for class priorities */
#define MSRP_SR_CLASS_A_PRIO	(3)
#define MSRP_SR_CLASS_B_PRIO	(2)
#define MSRP_SR_CLASS_C_PRIO	(2)

#define MSRP_SR_CLASS_VID		(2)

/* SR class interval frames definitions */
#define MSRP_SR_CLASS_A_INTERVAL_FRAMES		(8000)
#define MSRP_SR_CLASS_B_INTERVAL_FRAMES		(4000)
#define MSRP_SR_CLASS_C_INTERVAL_FRAMES		(1000)

#define MSRP_LISTENER_IGNORE    (0)
#define MSRP_LISTENER_ASKFAILED (1)
#define MSRP_LISTENER_READY     (2)
#define MSRP_LISTENER_READYFAIL (3)

#define MSRP_RANK_NON_EMERGENCY (1)
#define MSRP_RANK_EMERGENCY     (0)

#define MSRP_MAX_STREAMS (8)

#define MSRP_DEBUG (0)
#define DEBUG_PRINTF(frmt, args...)  do { if (MSRP_DEBUG > 0) printf(frmt, ## args); } while (0)

struct mrp_property {
	bool verbose;
	uint64_t streamid;
	uint64_t destaddr;
	int vlan;
	int MaxFrameSize;
	int MaxIntervalFrames;
	int priority;
	int rank;
	uint32_t latency;
	int class;
};

struct monitor_listener {
	bool     attached;
	uint64_t address;
};

struct msrp_ctx {
	int mrpd_sock;
	bool halt_flag;
	bool talker_found;
	int talker_leave;
	int listeners;
	pthread_t monitor_thread;
	struct mrp_property *prop;
	char *msgbuf;
	struct monitor_listener devices[MSRP_MAX_STREAMS];
};

extern struct msrp_ctx *msrp_ctx_init(struct mrp_property *prop);
extern int msrp_ctx_destroy(struct msrp_ctx *ctx);
extern int msrp_exist_listener(struct msrp_ctx *ctx);
extern bool msrp_exist_talker(struct msrp_ctx *ctx);
extern int mvrp_join_vlan(struct msrp_ctx *ctx);
extern int mvrp_leave_vlan(struct msrp_ctx *ctx);
extern int msrp_register_domain(struct msrp_ctx *ctx);
extern int msrp_unregister_domain(struct msrp_ctx *ctx);
extern int msrp_talker_advertise(struct msrp_ctx *ctx);
extern int msrp_talker_unadvertise(struct msrp_ctx *ctx);
extern int msrp_listener_ready(struct msrp_ctx *ctx);
extern int msrp_listener_leave(struct msrp_ctx *ctx);
extern int msrp_query_database(struct msrp_ctx *ctx);

#endif /* __MSRP_H__ */
