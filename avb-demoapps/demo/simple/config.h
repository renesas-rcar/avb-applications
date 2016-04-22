/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

/* IEEE 1722 reserved address */
#define DEST_ADDR { 0x91, 0xe0, 0xf0, 0x00, 0x0e, 0x80 }

#define TSOFFSET (2000ul)	/* us */

#define DEBUG_LEVEL		(1)

#define CONFIG_INIT_ENTRYNUM     (128)
#define CONFIG_INIT_PAYLOAD_SIZE (100)

#define MSRP_RANK (MSRP_RANK_NON_EMERGENCY)
#define LATENCY_TIME_MSRP (3900)

#define PRINTF3(frmt, args...)  do { if (DEBUG_LEVEL > 2) printf(frmt, ## args); } while (0)
#define PRINTF2(frmt, args...)  do { if (DEBUG_LEVEL > 1) printf(frmt, ## args); } while (0)
#define PRINTF1(frmt, args...)  do { if (DEBUG_LEVEL > 0) printf(frmt, ## args); } while (0)
#define PRINTF(frmt, args...)   printf(frmt, ## args)

#endif /* __CONFIG_H__ */
