/*
 * Copyright (c) 2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

#include "avdecc.h"

#define LIBVERSION "0.1"

void *avdecc_init(char *filename,
			 char *interface_name,
			 uint8_t role)
{
	return 0;
}

int avdecc_terminate(void *arg)
{
	return 0;
}

int avdecc_set_talker_stream_info(
	void *arg,
	uint16_t current_configuration,
	uint16_t unique_id,
	uint64_t stream_id,
	uint64_t stream_dest_mac,
	uint16_t stream_vlan_id)
{
	return 0;
}

int avdecc_get_connection_count(
	void *arg,
	uint16_t current_configuration,
	uint16_t unique_id)
{
	return 0;
}

int avdecc_get_connected_from_listener_stream_info(
	void *arg,
	uint16_t current_configuration,
	uint16_t unique_id)
{
	return 0;
}

int avdecc_get_current_configuration(void *arg)
{
	return 0;
}

int avdecc_get_stream_id_from_listener_stream_info(
	void *arg,
	uint16_t current_configuration,
	uint16_t unique_id,
	uint8_t *stream_id)
{
	return 0;
}

int avdecc_acmp_process_start(void *arg)
{
	return 0;
}

int avdecc_acmp_process_stop(void *arg)
{
	return 0;
}
