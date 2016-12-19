/*
 * Copyright (c) 2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef __AVDECC_H__
#define __AVDECC_H__

#include "jdksavdecc.h"

#define TIME_IN_MS_TO_WAIT  (1000)
#define VALID_TIME          (31)  /* IEEE1722.1-2013 Chapter.6.2.1.6 */
#define GPTP_GRANDMASTER_ID (0x0000000000000000UL)

#define AVDECC_DEBUG (0)
#define AVDECC_PRINTF3(frmt, args...)  do { if (AVDECC_DEBUG > 2) printf(frmt, ## args); } while (0)
#define AVDECC_PRINTF2(frmt, args...)  do { if (AVDECC_DEBUG > 1) printf(frmt, ## args); } while (0)
#define AVDECC_PRINTF1(frmt, args...)  do { if (AVDECC_DEBUG > 0) printf(frmt, ## args); } while (0)
#define AVDECC_PRINTF(frmt, args...)   printf(frmt, ## args)

#define DEBUG_ADP (0)
#define DEBUG_ADP_PRINTF3(frmt, args...)  do { if (DEBUG_ADP > 2) printf(frmt, ## args); } while (0)
#define DEBUG_ADP_PRINTF2(frmt, args...)  do { if (DEBUG_ADP > 1) printf(frmt, ## args); } while (0)
#define DEBUG_ADP_PRINTF1(frmt, args...)  do { if (DEBUG_ADP > 0) printf(frmt, ## args); } while (0)
#define DEBUG_ADP_PRINTF(frmt, args...)  printf(frmt, ## args)

#define DEBUG_AECP (0)
#define DEBUG_AECP_PRINTF3(frmt, args...)  do { if (DEBUG_AECP > 2) printf(frmt, ## args); } while (0)
#define DEBUG_AECP_PRINTF2(frmt, args...)  do { if (DEBUG_AECP > 1) printf(frmt, ## args); } while (0)
#define DEBUG_AECP_PRINTF1(frmt, args...)  do { if (DEBUG_AECP > 0) printf(frmt, ## args); } while (0)
#define DEBUG_AECP_PRINTF(frmt, args...)  printf(frmt, ## args)

#define DEBUG_ACMP (0)
#define DEBUG_ACMP_PRINTF3(frmt, args...)  do { if (DEBUG_ACMP > 2) printf(frmt, ## args); } while (0)
#define DEBUG_ACMP_PRINTF2(frmt, args...)  do { if (DEBUG_ACMP > 1) printf(frmt, ## args); } while (0)
#define DEBUG_ACMP_PRINTF1(frmt, args...)  do { if (DEBUG_ACMP > 0) printf(frmt, ## args); } while (0)
#define DEBUG_ACMP_PRINTF(frmt, args...)  printf(frmt, ## args)

#define ROLE_TALKER   0x01
#define ROLE_LISTENER 0x02

struct adp_state_data {
	uint64_t                last_time_in_microseconds;
	uint64_t                reannounce_timer_timeout;  /* IEEE1722.1-2013 Chapter.6.2.4.1.1 */
	bool                    do_advertise;              /* IEEE1722.1-2013 Chapter.6.2.5.1.6 */
	bool                    do_terminate;              /* IEEE1722.1-2013 Chapter.6.2.4.1.3 , 6.2.5.1.5 and 6.2.6.1.6*/
	bool                    do_discover;               /* IEEE1722.1-2013 Chapter.6.2.4.1.3 and 6.2.6.1.4*/
	bool                    link_is_up;                /* IEEE1722.1-2013 Chapter.6.2.5.1.7 */
	bool                    last_link_is_up;           /* IEEE1722.1-2013 Chapter.6.2.5.1.8 */
	bool                    rcvd_available;            /* IEEE1722.1-2013 Chapter.6.2.6.1.2 , not use */
	bool                    rcvd_departing;            /* IEEE1722.1-2013 Chapter.6.2.6.1.3 , not use */
	bool                    rcvd_discover;             /* IEEE1722.1-2013 Chapter.6.2.5.1.3 */
	struct jdksavdecc_eui64 advertised_grandmaster_id; /* IEEE1722.1-2013 Chapter.6.2.5.1.1 */
};

struct network_info {
	char      ifname[IFNAMSIZ];
	int       raw_sock;
	int       interface_id;
	uint16_t  ethertype;
	uint8_t   mac_addr[6];
};

struct avdecc_ctx {
	pthread_mutex_t         data_lock;
	uint8_t                 role;
	struct network_info     net_info;
	bool                    halt_flag;
	pthread_t               monitor_thread;
	uint64_t                timeout;
	struct jdksavdecc_eui64 entity_id;
	struct jdksavdecc_eui64 entity_model_id;
	struct jdksavdecc_adpdu adpdu;
	struct adp_state_data   adp_state;
	struct device_entity    *dev_e;
	bool                    acmp_started;
};

extern void *avdecc_init(char *filename,
			 char *interface_name,
			 uint8_t role);

extern int avdecc_terminate(void *arg);

extern int avdecc_set_talker_stream_info(
	void *arg,
	uint16_t current_configuration,
	uint16_t unique_id,
	uint64_t stream_id,
	uint64_t stream_dest_mac,
	uint16_t stream_vlan_id);

extern int avdecc_get_connection_count(
	void *arg,
	uint16_t current_configuration,
	uint16_t unique_id);

extern int avdecc_get_connected_from_listener_stream_info(
	void *arg,
	uint16_t current_configuration,
	uint16_t unique_id);

extern int avdecc_get_current_configuration(void *arg);

extern int avdecc_get_stream_id_from_listener_stream_info(
	void *arg,
	uint16_t current_configuration,
	uint16_t unique_id,
	uint8_t *stream_id);

extern int avdecc_acmp_process_start(void *arg);

extern int avdecc_acmp_process_stop(void *arg);

#endif /* __AVDECC_H__ */
