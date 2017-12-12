/*
 * Copyright (c) 2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef __AVDECC_INTERNAL_H__
#define __AVDECC_INTERNAL_H__

#include <stdio.h>
#include <stdint.h>
#include <net/if.h>

#include "jdksavdecc.h"
#include "avdecc.h"

#define TIME_IN_MS_TO_WAIT  (1000)
#define VALID_TIME          (31)  /* IEEE1722.1-2013 Chapter.6.2.1.6 */
#define GPTP_GRANDMASTER_ID (0x0000000000000000UL)
#define MAX_INFLIGHT_CMD_NUM (4)

#define ETHERNET_OVERHEAD  (14)
#define MAX_RECV_DATA_SIZE (2048)

#define ARRAY_SIZE(a)         (sizeof(a) / sizeof(a[0]))

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

/*
 * ADP defines
 */
struct adp_state_data {
	uint64_t                last_time_in_microseconds;
	uint64_t                reannounce_timer_timeout;  /* IEEE1722.1-2013 Chapter.6.2.4.1.1 */
	bool                    do_advertise;              /* IEEE1722.1-2013 Chapter.6.2.5.1.6 */
	bool                    do_terminate;              /* IEEE1722.1-2013 Chapter.6.2.4.1.3, 6.2.5.1.5 and 6.2.6.1.6 */
	bool                    do_discover;               /* IEEE1722.1-2013 Chapter.6.2.4.1.3 and 6.2.6.1.4 */
	bool                    link_is_up;                /* IEEE1722.1-2013 Chapter.6.2.5.1.7 */
	bool                    last_link_is_up;           /* IEEE1722.1-2013 Chapter.6.2.5.1.8 */
	bool                    rcvd_available;            /* IEEE1722.1-2013 Chapter.6.2.6.1.2, not use */
	bool                    rcvd_departing;            /* IEEE1722.1-2013 Chapter.6.2.6.1.3, not use */
	bool                    rcvd_discover;             /* IEEE1722.1-2013 Chapter.6.2.5.1.3 */
	struct jdksavdecc_eui64 advertised_grandmaster_id; /* IEEE1722.1-2013 Chapter.6.2.5.1.1 */
};

/*
 * ACMP defines
 */
/* IEEE1722.1-2013 Chapter.8.2.2.2.1 */
struct acmp_cmd_resp {
	uint8_t                 message_type;
	uint8_t                 status;
	struct jdksavdecc_eui64 stream_id;
	struct jdksavdecc_eui64 controller_entity_id;
	struct jdksavdecc_eui64 talker_entity_id;
	struct jdksavdecc_eui64 listener_entity_id;
	uint16_t                talker_unique_id;
	uint16_t                listener_unique_id;
	struct jdksavdecc_eui48 stream_dest_mac;
	uint16_t                connection_count;
	uint16_t                sequence_id;
	uint16_t                flags;
	uint16_t                stream_vlan_id;
};

/* IEEE1722.1-2013 Chapter.8.2.2.2.2 ListenerStreamInfo */
struct acmp_listener_stream_info {
	struct jdksavdecc_eui64 talker_entity_id;
	uint16_t                talker_unique_id;
	uint32_t                connected;
	struct jdksavdecc_eui64 stream_id;
	struct jdksavdecc_eui48 stream_dest_mac;
	struct jdksavdecc_eui64 controller_entity_id;
	uint16_t                flags;
	uint16_t                stream_vlan_id;

	/* library private */
	uint16_t                listener_unique_id;
};

/* IEEE1722.1-2013 Chapter.8.2.2.2.3 ListenerPair */
struct acmp_listener_pair {
	struct jdksavdecc_eui64 listener_entity_id;
	uint16_t                listener_unique_id;

	/* library private */
	struct acmp_listener_pair *next;
};

/* IEEE1722.1-2013 Chapter.8.2.2.2.4 TalkerStreamInfo */
struct acmp_talker_stream_info {
	struct jdksavdecc_eui64   stream_id;
	struct jdksavdecc_eui48   stream_dest_mac;
	uint16_t                  connection_count;
	struct acmp_listener_pair *connected_listeners;
	uint16_t                  stream_vlan_id;

	/* library private */
	uint16_t                  talker_unique_id;
};

/* IEEE1722.1-2013 Chapter.8.2.2.2.5 InflightCommand */
struct acmp_inflight_command {
	jdksavdecc_timestamp_in_microseconds timeout;
	bool                 retried;
	struct acmp_cmd_resp command;
	uint16_t             original_sequence_id;
};

/*
 * AVDECC Entity
 */
enum ENTITY_ACQUIRED_STATUS {
	ENTITY_NOT_ACQUIRED,
	ENTITY_ACQUIRED_PERSISTENT,
	ENTITY_PROVISIONAL_ACQUIRED,
	ENTITY_ACQUIRED,
};

enum ENTITY_LOCKED_STATUS {
	ENTITY_UNLOCKED,
	ENTITY_LOCKED,
};

struct exclusive_control_status {
	enum ENTITY_ACQUIRED_STATUS  acquired_status;
	struct jdksavdecc_eui64      acquired_owner_id;
	struct jdksavdecc_eui48      provisional_acquired_src_addr;
	struct jdksavdecc_eui64      provisional_acquired_owner_id;
	uint16_t                     provisional_sequence_id;
	uint16_t                     provisional_command_type;
	uint16_t                     provisional_command_index;
	uint64_t                     controller_available_timeout;
	enum ENTITY_LOCKED_STATUS    locked_status;
	struct jdksavdecc_eui64      locked_owner_id;
	uint64_t                     locked_timeout;
};

struct descriptor_entity
{
	/* IEEE1722.1-2013 Chapter.7.2.1 */
	struct jdksavdecc_descriptor_entity entity;
};

struct descriptor_counts_fmt {
	uint16_t descriptor_type;
	uint16_t count;
};

struct descriptor_configuration
{
	/* IEEE1722.1-2013 Chapter.7.2.2 */
	struct jdksavdecc_descriptor_configuration configuration;
	struct descriptor_counts_fmt *descriptor_counts; /* valiable */
};

struct descriptor_audio_unit
{
	/* IEEE1722.1-2013 Chapter.7.2.3 */
	struct jdksavdecc_descriptor_audio_unit audio_unit;
	struct jdksavdecc_values_sample_rate *sampling_rates; /* valiable */
};

struct descriptor_video_unit
{
	/* IEEE1722.1-2013 Chapter.7.2.4 */
	struct jdksavdecc_descriptor_video_unit video_unit;
};

struct descriptor_stream
{
	/* IEEE1722.1-2013 Chapter.7.2.6 */
	struct jdksavdecc_descriptor_stream stream;

	uint8_t  *formats; /* valiable */
};

struct descriptor_jack
{
	/* IEEE1722.1-2013 Chapter.7.2.7 */
	struct jdksavdecc_descriptor_jack jack;
};

struct descriptor_avb_interface
{
	/* IEEE1722.1-2013 Chapter.7.2.8 */
	struct jdksavdecc_descriptor_avb_interface avb_interface;
};

struct descriptor_clock_source
{
	/* IEEE1722.1-2013 Chapter.7.2.9 */
	struct jdksavdecc_descriptor_clock_source clock_source;
};

struct descriptor_locale
{
	/* IEEE1722.1-2013 Chapter.7.2.11 */
	struct jdksavdecc_descriptor_locale locale;
};

struct descriptor_strings
{
	/* IEEE1722.1-2013 Chapter.7.2.12 */
	struct jdksavdecc_descriptor_strings strings;
};

struct descriptor_stream_port
{
	/* IEEE1722.1-2013 Chapter.7.2.13 */
	struct jdksavdecc_descriptor_stream_port stream_port;

	uint8_t  *formats; /* valiable */
};

struct descriptor_audio_cluster
{
	/* IEEE1722.1-2013 Chapter.7.2.16 */
	struct jdksavdecc_descriptor_audio_cluster audio_cluster;

	uint64_t *formats; /* valiable */
};

struct descriptor_video_unit_cluster
{
	/* IEEE1722.1-2013 Chapter.7.2.17 */
	struct jdksavdecc_descriptor_video_unit_cluster video_unit_cluster;

	uint32_t *supported_format_specifics; /* valiable */
	uint32_t *supported_sampling_rates;   /* valiable */
	uint16_t *supported_aspect_ratios;    /* valiable */
	uint32_t *supported_sizes;            /* valiable */
	uint16_t *supported_color_spaces;     /* valiable */
};

struct descriptor_audio_map
{
	/* IEEE1722.1-2013 Chapter.7.2.19 */
	struct jdksavdecc_descriptor_audio_map audio_map;

	struct jdksavdecc_audio_mapping *mappings; /* valiable */
};

struct descriptor_video_unit_map
{
	/* IEEE1722.1-2013 Chapter.7.2.20 */
	struct jdksavdecc_descriptor_video_unit_map video_unit_map;

	struct jdksavdecc_video_mapping *mappings; /* valiable */
};

struct descriptor_clock_domain
{
	/* IEEE1722.1-2013 Chapter.7.2.32 */
	struct jdksavdecc_descriptor_clock_domain clock_domain;

	uint16_t *clock_sources; /*valiable*/
};

struct descriptor_data {
	union {
		struct descriptor_entity               entity_desc;
		struct descriptor_configuration        configuration_desc;
		struct descriptor_audio_unit           audio_unit_desc;
		struct descriptor_video_unit           video_unit_desc;
		struct descriptor_stream               stream_desc;
		struct descriptor_jack                 jack_desc;
		struct descriptor_avb_interface        avb_interface_desc;
		struct descriptor_clock_source         clock_source_desc;
		struct descriptor_locale               locale_desc;
		struct descriptor_strings              strings_desc;
		struct descriptor_stream_port          stream_port_desc;
		struct descriptor_audio_cluster        audio_cluster_desc;
		struct descriptor_video_unit_cluster   video_unit_cluster_desc;
		struct descriptor_audio_map            audio_map_desc;
		struct descriptor_video_unit_map       video_unit_map_desc;
		struct descriptor_clock_domain         clock_domain_desc;
	} desc;

	struct exclusive_control_status exclusive_ctl_status;
	struct descriptor_data *next;
};

struct configuration_list {
	struct descriptor_data           configuration;
	struct descriptor_data           *descriptor;
	struct acmp_listener_stream_info *acmp_listener_stream_infos;
	uint8_t                          listener_stream_infos_count;
	struct acmp_talker_stream_info   *acmp_talker_stream_infos;
	uint8_t                          talker_stream_infos_count;
	struct configuration_list        *next;
};

struct device_entity {
	struct descriptor_data       entity;
	struct configuration_list    *configuration_list;
	struct acmp_inflight_command acmp_listener_inflight_commands[MAX_INFLIGHT_CMD_NUM];
};

/*
 * libavdecc defines
 */
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

/*
 * ADP functions
 */
int adp_advertise_init(struct avdecc_ctx *ctx);

int adp_receive_process(
	struct jdksavdecc_eui64 *entity_id,
	struct adp_state_data *adp_state,
	const struct jdksavdecc_frame *frame);

int adp_state_process(struct avdecc_ctx *ctx);

int adp_state_change_departing(struct adp_state_data *adp_state);

int adp_set_grandmaster_id(struct avdecc_ctx *ctx, uint64_t grandmaster_id);

/*
 * ACMP functions
 */
int acmp_ctx_init(struct avdecc_ctx *ctx);

int acmp_set_streamid(
	struct configuration_list *configuration_list,
	uint16_t current_configuration,
	uint16_t unique_id,
	uint64_t stream_id);

int acmp_set_stream_dest_mac(
	struct configuration_list *configuration_list,
	uint16_t current_configuration,
	uint16_t unique_id,
	uint64_t stream_dest_mac);

int acmp_set_stream_vlan_id(
	struct configuration_list *configuration_list,
	uint16_t current_configuration,
	uint16_t unique_id,
	uint16_t stream_vlan_id);

int acmp_get_connection_count(
	struct configuration_list *configuration_list,
	uint16_t current_configuration,
	uint16_t unique_id);

int acmp_get_connected_from_listener_stream_info(
	struct configuration_list *configuration_list,
	uint16_t current_configuration,
	uint16_t unique_id);

int acmp_get_stream_id_from_listener_stream_info(
	struct configuration_list *configuration_list,
	uint16_t current_configuration,
	uint16_t unique_id,
	uint8_t *stream_id);

int acmp_ctx_terminate(struct configuration_list *config_list);

int acmp_receive_process(struct avdecc_ctx *ctx,
			 const struct jdksavdecc_frame *frame);

int acmp_state_process(struct avdecc_ctx *ctx);

/*
 * AECP functions
 */
int aecp_receive_process(
	struct avdecc_ctx *ctx,
	const struct jdksavdecc_frame *frame);

int aecp_state_process(struct avdecc_ctx *ctx);

/*
 * AVDECC Entity
 */
struct device_entity *read_device_entity(const char *filename);
int release_device_entity(struct device_entity *dev_e);
int setup_descriptor_data(struct device_entity *dev_e, uint8_t *mac_addr);

/*
 * Common Utility
 */
int is_interface_linkup(int sock, const char *interface_name);

void set_socket_nonblocking(int fd);

int send_data(
	struct network_info *net_info,
	const uint8_t dest_mac[6],
	const void *payload,
	ssize_t payload_len);

void close_socket(int fd);

int create_socket(struct network_info *net_info, const char *interface_name);

#endif /* __AVDECC_INTERNAL_H__ */
