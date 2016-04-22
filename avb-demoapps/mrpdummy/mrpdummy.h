/*
 * Copyright (c) 2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#ifndef __MRPDUMMY_H__
#define __MRPDUMMY_H__

/******************/
/* default number */
/******************/
#define MRPDUMMY_FRAME_SIZE_DEFAULT     (84)
#define MRPDUMMY_INTERVAL_FRAME_DEFAULT (1)

#define STREAM_ID (0x0000000000000000)
#define DEST_ADDR (0x91e0f0000e80)

#define MSRP_RANK (MSRP_RANK_NON_EMERGENCY)
#define LATENCY_TIME_MSRP (3900)

#define MODE_TALKER            (0)
#define MODE_LISTENER          (1)
#define MODE_JOIN_VLAN         (2)
#define MODE_LEAVE_VLAN        (3)
#define MODE_REGISTER_DOMAIN   (4)
#define MODE_UNREGISTER_DOMAIN (5)

#define DEBUG_LEVEL		(0)

#define PRINTF3(frmt, args...)  do { if (DEBUG_LEVEL > 2) printf(frmt, ## args); } while (0)
#define PRINTF2(frmt, args...)  do { if (DEBUG_LEVEL > 1) printf(frmt, ## args); } while (0)
#define PRINTF1(frmt, args...)  do { if (DEBUG_LEVEL > 0) printf(frmt, ## args); } while (0)
#define PRINTF(frmt, args...)   printf(frmt, ## args)

#endif /* __MRPDUMMY_H__ */
