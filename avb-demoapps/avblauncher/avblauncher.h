/*
 * Copyright (c) 2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#ifndef __AVBLAUNCHER_H__
#define __AVBLAUNCHER_H__

#include <net/if.h>
#include <linux/if_ether.h>
#include "avtp.h"

#define READ  (0)
#define WRITE (1)

#define CMDNUM (30)

#define INI_SECTION_NAME "avblauncher"
#define GPTP_DAEMON_CMD  "daemon_cl"
#define SRP_DAEMON_CMD   "mrpd"
#define MAAP_DAEMON_CMD  "maap_daemon"
#define MRPDUMMY_CMD     "mrpdummy"

#define MAXSTR  (INI_MAX_LINE)
#define DEAMON_CL_SHM_SIZE (sizeof(struct gptp_time_data) + sizeof(pthread_mutex_t))
#define DAEMON_CL_SHM_NAME "/ptp"
#define DAEMON_CL_MAX_RETRY_NUM (5)

#define MAAP_DAEMON_SHM_SIZE (sizeof(uint8_t) * ETH_ALEN)
#define MAAP_DAEMON_SHM_KEY 1234
#define MAAP_DAEMON_MAX_RETRY_NUM (5)

#define DEBUG_LEVEL (1)
#define PRINTF3(frmt, args...)  do { if (DEBUG_LEVEL > 2) printf(frmt, ## args); } while (0)
#define PRINTF2(frmt, args...)  do { if (DEBUG_LEVEL > 1) printf(frmt, ## args); } while (0)
#define PRINTF1(frmt, args...)  do { if (DEBUG_LEVEL > 0) printf(frmt, ## args); } while (0)
#define PRINTF(frmt, args...)   printf(frmt, ## args)

#define SLEEP_TIME (100000)

#define INI_SUCCESS  (1)
#define INI_FAILURE  (0)

#define INI_MODE            "MODE"
#define INI_GPTP            "GPTP"
#define INI_SRP             "SRP"
#define INI_AVDECC          "AVDECC"
#define INI_DEST_ADDR       "DEST_ADDR"
#define INI_STREAM_ID       "STREAM_ID"
#define INI_APPLICATION     "APPLICATION"
#define INI_SR_CLASS        "SR_CLASS"
#define INI_SR_PRIORITY     "SR_PRIORITY"
#define INI_VLAN_ID         "VLAN_ID"
#define INI_UNIQUE_ID       "UNIQUE_ID"
#define INI_MAX_FRAMESIZE   "MAX_FRAMESIZE"
#define INI_FRAME_INTERVALS "FRAME_INTERVALS"
#define INI_LATENCY         "LATENCY"

enum MODE_TYPE {
	LISTENER_MODE,
	TALKER_MODE
};

struct gptp_time_data {
	int64_t     ml_phoffset;
	int64_t     ls_phoffset;
	long double ml_freqoffset;
	long double ls_freqoffset;
	uint64_t    local_time;
	uint32_t    sync_count;
	uint32_t    pdelay_count;
	bool        ascapable;
	int         port_state;
	pid_t       process_id;
};

enum INDEX_CONTEXT_MEMBER {
	IDX_OPERATING_MODE,
	IDX_USE_GPTP,
	IDX_USE_SRP,
	IDX_USE_AVDECC,
	IDX_SR_CLASS,
	IDX_SR_PRIORITY,
	IDX_VID,
	IDX_UID,
	IDX_STREAM_ID,
	IDX_DEST_ADDR,
	IDX_MAXFRAMESIZE,
	IDX_FRAMEINTERVALS,
	IDX_LATENCY,
	IDX_APPLICATION,
	IDX_MAX_NUM
};

struct app_context {
	enum MODE_TYPE operating_mode;
	bool           use_gptp;
	bool           use_srp;
	bool           use_maap;
	bool           use_avdecc;
	uint64_t       srclass;
	uint64_t       srpriority;
	uint64_t       vid;
	uint64_t       uid;
	uint8_t        streamid[AVTP_STREAMID_SIZE];
	uint8_t        src_addr[ETH_ALEN];
	uint8_t        dest_addr[ETH_ALEN];
	uint64_t       maxframesize;
	uint64_t       frameintervals;
	uint64_t       latency;
	pid_t          mrpdummy_pid;
	pid_t          app_pid;
	char           app_cmd[MAXSTR];
	bool           err_flag;
	pthread_t      thread;
	char           *gptp_shm_addr;
	char           *maap_shm_addr;
	bool           required[IDX_MAX_NUM];
	char           *ini_name;
	char           avdecc_entity[MAXSTR];
	void           *avdecc;
	char           eth_interface[MAXSTR];
};

struct domain_info {
	int srclass;
	int srpriority;
	struct domain_info *next;
};


struct connect_info {
	int vid;
	struct domain_info *domain;
	struct connect_info *next;
};

#endif /* __AVBLAUNCHER_H__ */
