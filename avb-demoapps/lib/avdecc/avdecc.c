/*
 * Copyright (c) 2016-2017 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#include <pthread.h>
#include <linux/if_ether.h>

#include "avdecc_internal.h"

#define LIBVERSION "0.1"

static int protocol_process(
	struct avdecc_ctx *ctx,
	const struct jdksavdecc_frame *frame)
{
	int rc = 0;

	pthread_mutex_lock(&ctx->data_lock);
	if (frame->length > 0) {
		switch (frame->payload[0]) {
		case JDKSAVDECC_1722A_SUBTYPE_ADP:
			AVDECC_PRINTF2("[AVDECC] ADP response process\n");
			rc = adp_receive_process(&ctx->entity_id, &ctx->adp_state, frame);
			break;
		case JDKSAVDECC_1722A_SUBTYPE_AECP:
			AVDECC_PRINTF2("[AVDECC] AEMP response process\n");
			rc = aecp_receive_process(ctx, frame);
			break;
		case JDKSAVDECC_1722A_SUBTYPE_ACMP:
			if (ctx->acmp_started) {
				AVDECC_PRINTF2("[AVDECC] ACMP response process\n");
				rc = acmp_receive_process(ctx, frame);
			}
			break;
		default:
			/* other subtype, ignore it */
			break;
		}
	}
	if (rc < 0)
		return -1;

	if (ctx->halt_flag)
		adp_state_change_departing(&ctx->adp_state);

	rc = adp_state_process(ctx);
	if (rc < 0)
		return -1;

	rc = aecp_state_process(ctx);
	if (rc < 0)
		return -1;

	if (ctx->acmp_started) {
		rc = acmp_state_process(ctx);
		if (rc < 0)
			return -1;
	}

	if (ctx->halt_flag)
		return 1;
	pthread_mutex_unlock(&ctx->data_lock);

	return 0;
}

static void *avdecc_monitor_thread(void *arg)
{
	int rc = 0;
	fd_set rfds;
	int fds;
	struct avdecc_ctx *ctx = NULL;
	struct timeval tv;
	char buf[MAX_RECV_DATA_SIZE];
	struct jdksavdecc_frame frame;
	struct timeval time_portion;

	if (!arg) {
		AVDECC_PRINTF1("[AVDECC] arg is NULL.\n");
		pthread_exit(NULL);
	}
	ctx = (struct avdecc_ctx *)arg;

	set_socket_nonblocking(ctx->net_info.raw_sock);
	FD_ZERO(&rfds);
	FD_SET(ctx->net_info.raw_sock, &rfds);
	fds = ctx->net_info.raw_sock + 1;

	tv.tv_sec = ctx->timeout / 1000;
	tv.tv_usec = (ctx->timeout % 1000) * 1000;

	AVDECC_PRINTF2("[AVDECC] avdecc_monitor_thread started\n");

	while (1) {
		time_portion = tv;

		FD_SET(ctx->net_info.raw_sock, &rfds);

		rc = select(fds, &rfds, NULL, NULL, &time_portion);
		if (rc < 0) {
			perror("[AVDECC] select:");
			break;
		}

		if (rc > 0 && (FD_ISSET(ctx->net_info.raw_sock, &rfds))) {
			int len;

			bzero(&frame, sizeof(frame));

			len = recv(ctx->net_info.raw_sock, buf, MAX_RECV_DATA_SIZE, 0);
			if (len >= 0) {
				memcpy(frame.dest_address.value, &(buf[0]), ETH_ALEN);
				memcpy(frame.src_address.value, &(buf[6]), ETH_ALEN);
				frame.ethertype = ((buf[7] << 8) | buf[8]);

				if (frame.payload && (sizeof(frame.payload) > len - ETHERNET_OVERHEAD)) {
					memcpy(frame.payload,
					       &buf[ETHERNET_OVERHEAD],
					       len - ETHERNET_OVERHEAD);

					frame.length = len - ETHERNET_OVERHEAD;
				}
			}
		} else {
			frame.length = 0;
		}

		rc = protocol_process(ctx, &frame);
		if (rc < 0) {
			AVDECC_PRINTF1("[AVDECC] error in protocol_process().\n");
			break;
		} else if (rc > 0) {
			break;
		}
	}

	AVDECC_PRINTF2("[AVDECC] avdecc_monitor_thread halted\n");
	pthread_exit(NULL);
}

static int avdecc_release_resource(struct avdecc_ctx *ctx)
{
	if (!ctx)
		return -1;

	if (ctx->dev_e) {
		if (ctx->dev_e->configuration_list)
			acmp_ctx_terminate(ctx->dev_e->configuration_list);

		release_device_entity(ctx->dev_e);
	}

	close_socket(ctx->net_info.raw_sock);

	free(ctx);

	return 0;
}

void *avdecc_init(
	char *yaml_filename,
	char *interface_name,
	uint8_t role)
{
	int rc;
	struct avdecc_ctx *ctx;

	if (!yaml_filename) {
		AVDECC_PRINTF1("[AVDECC] yaml_filename is NULL.\n");
		return NULL;
	}

	if (!interface_name) {
		AVDECC_PRINTF1("[AVDECC] interface_name is NULL.\n");
		return NULL;
	}

	ctx = calloc(1, sizeof(struct avdecc_ctx));
	if (!ctx) {
		AVDECC_PRINTF1("[AVDECC] Failed to allocate context.\n");
		return NULL;
	}
	pthread_mutex_init(&ctx->data_lock, NULL);
	ctx->role = role;
	ctx->halt_flag = false;
	ctx->net_info.ethertype = JDKSAVDECC_AVTP_ETHERTYPE;
	ctx->timeout = TIME_IN_MS_TO_WAIT;

	rc = create_socket(&ctx->net_info, interface_name);
	if (rc < 0) {
		free(ctx);
		AVDECC_PRINTF1("[AVDECC] Failed to create socket.\n");
		return NULL;
	}

	ctx->dev_e = read_device_entity(yaml_filename);
	if (!ctx->dev_e) {
		avdecc_release_resource(ctx);
		AVDECC_PRINTF1("[AVDECC] Failed to read device entity information.\n");
		return NULL;
	}

	rc = setup_descriptor_data(ctx->dev_e, ctx->net_info.mac_addr);
	if (rc < 0) {
		avdecc_release_resource(ctx);
		AVDECC_PRINTF1("[AVDECC] Failed to create socket.\n");
		return NULL;
	}

	jdksavdecc_eui64_copy(
			&ctx->entity_id,
			&ctx->dev_e->entity.desc.entity_desc.entity.entity_id);

	rc = adp_advertise_init(ctx);
	if (rc < 0) {
		avdecc_release_resource(ctx);
		AVDECC_PRINTF1("[AVDECC] Failed to init ADP parameter in ctx.\n");
		return NULL;
	}

	rc = acmp_ctx_init(ctx);
	if (rc < 0) {
		avdecc_release_resource(ctx);
		AVDECC_PRINTF1("[AVDECC] Failed to int ACMP parameter in ctx.\n");
		return NULL;
	}

	rc = pthread_create(&ctx->monitor_thread, NULL, avdecc_monitor_thread, (void *)ctx);
	if (rc) {
		avdecc_release_resource(ctx);
		AVDECC_PRINTF1("[AVDECC] Failed to create thread.\n");
		return NULL;
	}

	return ctx;
}

int avdecc_terminate(void *arg)
{
	struct avdecc_ctx *ctx;

	if (!arg) {
		AVDECC_PRINTF1("[AVDECC] avdecc_terminate(): arg is NULL.\n");
		return -1;
	}

	ctx = (struct avdecc_ctx *)arg;
	ctx->halt_flag = true;

	if (ctx->monitor_thread)
		pthread_join(ctx->monitor_thread, NULL);

	avdecc_release_resource(ctx);

	return 0;
}

int avdecc_set_grandmaster_id(void *arg, uint64_t grandmaster_id)
{
	struct avdecc_ctx *ctx;

	if (!arg) {
		AVDECC_PRINTF1("[AVDECC] avdecc_set_grandmaster_id(): arg is NULL.\n");
		return -1;
	}

	ctx = (struct avdecc_ctx *)arg;

	return adp_set_grandmaster_id(ctx, grandmaster_id);
}

int avdecc_set_talker_stream_info(
	void *arg,
	uint16_t current_configuration,
	uint16_t unique_id,
	uint64_t stream_id,
	uint64_t stream_dest_mac,
	uint16_t stream_vlan_id)
{
	int rc = 0;
	struct avdecc_ctx *ctx;

	if (!arg) {
		AVDECC_PRINTF1("[AVDECC] avdecc_terminate(): arg is NULL.\n");
		return -1;
	}

	ctx = (struct avdecc_ctx *)arg;

	pthread_mutex_lock(&ctx->data_lock);
	rc = acmp_set_streamid(
			ctx->dev_e->configuration_list,
			current_configuration,
			unique_id,
			stream_id);
	if (rc < 0) {
		AVDECC_PRINTF1("[AVDECC] avdecc_set_talker_stream_info(): Could not set stream_id.\n");
		return -1;
	}

	rc = acmp_set_stream_dest_mac(
			ctx->dev_e->configuration_list,
			current_configuration,
			unique_id,
			stream_dest_mac);
	if (rc < 0) {
		AVDECC_PRINTF1("[AVDECC] avdecc_set_talker_stream_info(): Could not set stream_dest_mac.\n");
		return -1;
	}

	rc = acmp_set_stream_vlan_id(
			ctx->dev_e->configuration_list,
			current_configuration,
			unique_id,
			stream_vlan_id);
	if (rc < 0) {
		AVDECC_PRINTF1("[AVDECC] avdecc_set_talker_stream_info(): Could not set stream_vlan_id.\n");
		return -1;
	}
	pthread_mutex_unlock(&ctx->data_lock);

	return 0;
}

int avdecc_get_connection_count(
	void *arg,
	uint16_t current_configuration,
	uint16_t unique_id)
{
	struct avdecc_ctx *ctx;

	if (!arg) {
		AVDECC_PRINTF1("[AVDECC] avdecc_get_connection_count(): arg is NULL.\n");
		return -1;
	}

	ctx = (struct avdecc_ctx *)arg;

	return acmp_get_connection_count(ctx->dev_e->configuration_list, current_configuration, unique_id);
}

int avdecc_get_current_configuration(void *arg)
{
	struct avdecc_ctx *ctx;

	if (!arg) {
		AVDECC_PRINTF1("[AVDECC] avdecc_get_current_configuration(): arg is NULL.\n");
		return -1;
	}

	ctx = (struct avdecc_ctx *)arg;

	return ctx->dev_e->entity.desc.entity_desc.entity.current_configuration;
}

int avdecc_get_connected_from_listener_stream_info(
		void *arg,
		uint16_t current_configuration,
		uint16_t unique_id)
{
	struct avdecc_ctx *ctx;

	if (!arg) {
		AVDECC_PRINTF1("[AVDECC] avdecc_get_connected_from_listener_stream_info(): arg is NULL.\n");
		return -1;
	}

	ctx = (struct avdecc_ctx *)arg;

	return acmp_get_connected_from_listener_stream_info(
			ctx->dev_e->configuration_list,
			current_configuration,
			unique_id);
}

int avdecc_acmp_process_start(void *arg)
{
	struct avdecc_ctx *ctx;

	if (!arg) {
		AVDECC_PRINTF1("[AVDECC] avdecc_acmp_process_start(): arg is NULL.\n");
		return -1;
	}

	ctx = (struct avdecc_ctx *)arg;

	pthread_mutex_lock(&ctx->data_lock);
	ctx->acmp_started = true;
	pthread_mutex_unlock(&ctx->data_lock);

	return 0;
}

int avdecc_acmp_process_stop(void *arg)
{
	struct avdecc_ctx *ctx;

	if (!arg) {
		AVDECC_PRINTF1("[AVDECC] avdecc_acmp_process_stop(): arg is NULL.\n");
		return -1;
	}

	ctx = (struct avdecc_ctx *)arg;

	pthread_mutex_lock(&ctx->data_lock);
	ctx->acmp_started = false;
	pthread_mutex_unlock(&ctx->data_lock);

	return 0;
}

int avdecc_get_stream_id_from_listener_stream_info(void *arg, uint16_t current_configuration, uint16_t unique_id, uint8_t *stream_id)
{
	struct avdecc_ctx *ctx;

	if (!arg) {
		AVDECC_PRINTF1("[AVDECC] avdecc_get_stream_id_from_listener_stream_info(): arg is NULL.\n");
		return -1;
	}

	ctx = (struct avdecc_ctx *)arg;

	return acmp_get_stream_id_from_listener_stream_info(ctx->dev_e->configuration_list, current_configuration, unique_id, stream_id);
}



