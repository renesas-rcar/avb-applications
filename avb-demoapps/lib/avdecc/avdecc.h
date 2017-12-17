/*
 * Copyright (c) 2016-2017 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef __AVDECC_H__
#define __AVDECC_H__

#define ROLE_TALKER   0x01
#define ROLE_LISTENER 0x02

/*
 * AVDECC init/terminate
 */
extern void *avdecc_init(char *filename,
			 char *interface_name,
			 uint8_t role);

extern int avdecc_terminate(void *arg);

/*
 * ADP set Grandmaster ID
 */
extern int avdecc_set_grandmaster_id(void *arg, uint64_t grandmaster_id);

/*
 * AECP get current configuration
 */
extern int avdecc_get_current_configuration(void *arg);

/*
 * ACMP talker_stream_info
 */
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

/*
 * ACMP listener_stream_info
 */
extern int avdecc_get_connected_from_listener_stream_info(
	void *arg,
	uint16_t current_configuration,
	uint16_t unique_id);

extern int avdecc_get_stream_id_from_listener_stream_info(
	void *arg,
	uint16_t current_configuration,
	uint16_t unique_id,
	uint8_t *stream_id);

/*
 * ACMP start/stop
 */
extern int avdecc_acmp_process_start(void *arg);

extern int avdecc_acmp_process_stop(void *arg);

#endif /* __AVDECC_H__ */
