/*
 * Copyright (c) 2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#include <time.h>

#include "avdecc_internal.h"

/* IEEE1722.1-2013 Chapter.6.2.5.2.1 */
static void adp_advertise_tx_entity_available(struct avdecc_ctx *ctx)
{
	struct jdksavdecc_frame frame;

	jdksavdecc_frame_init(&frame);

	ctx->adpdu.header.message_type =
		JDKSAVDECC_ADP_MESSAGE_TYPE_ENTITY_AVAILABLE;

	frame.length = jdksavdecc_adpdu_write(&ctx->adpdu,
					      &frame.payload,
					      0,
					      sizeof(frame.payload));
	frame.dest_address = jdksavdecc_multicast_adp_acmp;

	if (frame.length > 0) {
		if (send_data(&ctx->net_info,
			      frame.dest_address.value,
			      frame.payload,
			      frame.length) > 0) {
			DEBUG_ADP_PRINTF2("[ADP] Sent: ADP_AVAILABLE\n");
		}

		ctx->adpdu.available_index++;
	}
}

/* IEEE1722.1-2013 Chapter.6.2.5.2.2 */
static void adp_advertise_tx_entity_departing(struct avdecc_ctx *ctx)
{
	struct jdksavdecc_frame frame;

	jdksavdecc_frame_init(&frame);

	ctx->adpdu.header.message_type =
		JDKSAVDECC_ADP_MESSAGE_TYPE_ENTITY_DEPARTING;

	frame.length = jdksavdecc_adpdu_write(&ctx->adpdu,
					      &frame.payload,
					      0,
					      sizeof(frame.payload));
	frame.dest_address = jdksavdecc_multicast_adp_acmp;

	if (frame.length > 0) {
		if (send_data(&ctx->net_info,
			      frame.dest_address.value,
			      frame.payload,
			      frame.length) > 0) {
			DEBUG_ADP_PRINTF2("[ADP] Sent: ADP_DEPARTINGn");
		}

		ctx->adpdu.available_index = 0;
	}
}

/* IEEE1722.1-2013 Chapter.6.2.6.3.1 */
static void adp_advertise_tx_discover(struct avdecc_ctx *ctx)
{
	struct jdksavdecc_frame frame;
	struct jdksavdecc_adpdu adpdu;

	jdksavdecc_frame_init(&frame);

	memset(&adpdu, 0, sizeof(adpdu));

	adpdu.header.cd = true;
	adpdu.header.subtype = JDKSAVDECC_SUBTYPE_ADP;
	adpdu.header.control_data_length =
		JDKSAVDECC_ADPDU_LEN - JDKSAVDECC_COMMON_CONTROL_HEADER_LEN;
	adpdu.header.message_type =
		JDKSAVDECC_ADP_MESSAGE_TYPE_ENTITY_DISCOVER;
	adpdu.header.sv = 0;
	adpdu.header.version = 0;
	adpdu.header.valid_time = 0;
	jdksavdecc_eui64_init_from_uint64(&adpdu.header.entity_id, 0);

	frame.length = jdksavdecc_adpdu_write(&adpdu,
					      &frame.payload,
					      0,
					      sizeof(frame.payload));
	frame.dest_address = jdksavdecc_multicast_adp_acmp;

	if (frame.length > 0) {
		if (send_data(&ctx->net_info,
			      frame.dest_address.value,
			      frame.payload,
			      frame.length) > 0) {
			DEBUG_ADP_PRINTF2("[ADP] Sent: ADP_DISCOVER\n");
		}
	}
}

/*
 * public functions
 */
int adp_advertise_init(struct avdecc_ctx *ctx)
{
	struct jdksavdecc_eui64 zero;
	int link_up;
	if (!ctx) {
		DEBUG_ADP_PRINTF1(
			"[ADP] ctx is NULL in adp_advertise_init()\n");
		return -1;
	}

	if (!ctx->dev_e) {
		DEBUG_ADP_PRINTF1(
			"[ADP] ctx->dev_e is NULL in adp_advertise_init()\n");
		return -1;
	}

	bzero(&zero, sizeof(zero));

	ctx->adp_state.last_time_in_microseconds = 0;
	ctx->adp_state.do_advertise = 1;
	ctx->adp_state.do_terminate = 0;
	ctx->adp_state.advertised_grandmaster_id = zero; /* check */
	ctx->adp_state.rcvd_discover = 0;

	ctx->adp_state.link_is_up = 0;
	link_up = is_interface_linkup(ctx->net_info.raw_sock, ctx->net_info.ifname);
	if (link_up < 0) {
		DEBUG_ADP_PRINTF1(
			"[ADP] Error : is_interface_linkup()\n");
		return -1;
	}
	ctx->adp_state.last_link_is_up = link_up;

	memset(&ctx->adpdu, 0, sizeof(ctx->adpdu));
	ctx->adpdu.header.cd = true;
	ctx->adpdu.header.subtype = JDKSAVDECC_SUBTYPE_ADP;
	ctx->adpdu.header.sv = 0;
	ctx->adpdu.header.version = 0;
	ctx->adpdu.header.message_type =
		JDKSAVDECC_ADP_MESSAGE_TYPE_ENTITY_AVAILABLE;
	ctx->adpdu.header.valid_time = VALID_TIME;
	ctx->adpdu.header.control_data_length =
		JDKSAVDECC_ADPDU_LEN - JDKSAVDECC_COMMON_CONTROL_HEADER_LEN;

	jdksavdecc_eui64_copy(&ctx->adpdu.header.entity_id, &ctx->entity_id);
	jdksavdecc_eui64_copy(&ctx->adpdu.entity_model_id,
		&ctx->dev_e->entity.desc.entity_desc.entity.entity_model_id);

	ctx->adpdu.entity_capabilities =
		ctx->dev_e->entity.desc.entity_desc.entity.entity_capabilities;
	ctx->adpdu.talker_stream_sources =
		ctx->dev_e->entity.desc.entity_desc.entity.talker_stream_sources;
	ctx->adpdu.talker_capabilities =
		ctx->dev_e->entity.desc.entity_desc.entity.talker_capabilities;
	ctx->adpdu.listener_stream_sinks =
		ctx->dev_e->entity.desc.entity_desc.entity.listener_stream_sinks;
	ctx->adpdu.listener_capabilities =
		ctx->dev_e->entity.desc.entity_desc.entity.listener_capabilities;
	ctx->adpdu.controller_capabilities =
		ctx->dev_e->entity.desc.entity_desc.entity.controller_capabilities;
	ctx->adpdu.available_index = 0x0;
	jdksavdecc_eui64_init_from_uint64(&ctx->adpdu.gptp_grandmaster_id,
					  GPTP_GRANDMASTER_ID);
	ctx->adpdu.gptp_domain_number = 0x00;
	ctx->adpdu.reserved0 = 0;
	ctx->adpdu.identify_control_index = 0;
	ctx->adpdu.interface_index = 0;
	ctx->adpdu.association_id = zero;
	ctx->adpdu.reserved1 = 0;

	return 0;
}

int adp_receive_process(struct jdksavdecc_eui64 *entity_id,
			struct adp_state_data *adp_state,
			const struct jdksavdecc_frame *frame)
{
	struct jdksavdecc_adpdu adpdu;
	struct jdksavdecc_eui64 zero;

	if (!entity_id) {
		DEBUG_ADP_PRINTF1(
			"[ADP] entity_id is NULL in adp_receive_process().\n");
		return -1;
	}

	if (!adp_state) {
		DEBUG_ADP_PRINTF1(
			"[ADP] adp_state is NULL in adp_receive_process().\n");
		return -1;
	}

	if (!frame) {
		DEBUG_ADP_PRINTF1(
			"[ADP] frame is NULL in adp_receive_process().\n");
		return -1;
	}

	bzero(&zero, sizeof(zero));

	if (jdksavdecc_adpdu_read(&adpdu,
				  frame->payload,
				  0,
				  frame->length) > 0) {
		switch (adpdu.header.message_type) {
		case JDKSAVDECC_ADP_MESSAGE_TYPE_ENTITY_DISCOVER:
			DEBUG_ADP_PRINTF2("[ADP] Received ENTITY_DISCOVER:\n");
			adp_state->rcvd_discover = 1;
			if (!jdksavdecc_eui64_compare(&adpdu.header.entity_id, &zero) ||
			    !jdksavdecc_eui64_compare(&adpdu.header.entity_id, entity_id))
				adp_state->do_advertise = 1;
			adp_state->rcvd_discover = 0;
			break;
		case JDKSAVDECC_ADP_MESSAGE_TYPE_ENTITY_AVAILABLE:
			DEBUG_ADP_PRINTF2("[ADP] Received ENTITY_AVAILABLE:\n");
			/* for controller process */
			adp_state->rcvd_available = 0;
			break;
		case JDKSAVDECC_ADP_MESSAGE_TYPE_ENTITY_DEPARTING:
			DEBUG_ADP_PRINTF2("[ADP] Received ENTITY_DEPARTING:\n");
			/* for controller process */
			adp_state->rcvd_departing = 0;
			break;
		default:
			break;
		}
	}

	return 0;
}

int adp_state_process(struct avdecc_ctx *ctx)
{
	struct timespec t;
	int link_up;
	jdksavdecc_timestamp_in_microseconds current_time_in_microseconds;
	jdksavdecc_timestamp_in_microseconds diff_time;
	jdksavdecc_timestamp_in_microseconds valid_time_in_microseconds;

	if (!ctx) {
		DEBUG_ADP_PRINTF1(
			"[ADP] ctx is NULL in adp_state_process().\n");
		return -1;
	}

	clock_gettime(CLOCK_MONOTONIC, &t);
	current_time_in_microseconds =
		(t.tv_sec * 1000000) + (t.tv_nsec / 1000);

	diff_time = current_time_in_microseconds -
			ctx->adp_state.last_time_in_microseconds;
	valid_time_in_microseconds =
		(ctx->adpdu.header.valid_time * 1000000 * 2) / 4;

	if (valid_time_in_microseconds < 1000000)
		valid_time_in_microseconds = 1000000;

	if (diff_time > valid_time_in_microseconds) {
		DEBUG_ADP_PRINTF2("[ADP] reannounceTimerTimeout\n");
		ctx->adp_state.do_advertise = 1;
	}

	link_up = is_interface_linkup(ctx->net_info.raw_sock, ctx->net_info.ifname);
	if (link_up < 0) {
		DEBUG_ADP_PRINTF1(
			"[ADP] Error : process: is_interface_linkup()\n");
		return -1;
	}
	ctx->adp_state.link_is_up = link_up;

	if (ctx->adp_state.last_link_is_up !=
		ctx->adp_state.link_is_up) {
		DEBUG_ADP_PRINTF2("[ADP] link state changed\n");
		ctx->adp_state.last_link_is_up = ctx->adp_state.link_is_up;
		if (ctx->adp_state.link_is_up)
			ctx->adp_state.do_advertise = 1;
	}

	/* If different currentGrandmasterID and advertisedGrandmasterID */
	if (jdksavdecc_eui64_compare(&ctx->adpdu.gptp_grandmaster_id,
				     &ctx->adp_state.advertised_grandmaster_id)) {
		DEBUG_ADP_PRINTF2("[ADP] GM Update\n");
		jdksavdecc_eui64_copy(&ctx->adp_state.advertised_grandmaster_id,
				      &ctx->adpdu.gptp_grandmaster_id);
		ctx->adp_state.do_advertise = 1;
	}

	if (ctx->adp_state.do_advertise) {
		DEBUG_ADP_PRINTF2("[ADP] do_advertise\n");
		ctx->adp_state.last_time_in_microseconds =
			current_time_in_microseconds;
		adp_advertise_tx_entity_available(ctx);
		ctx->adp_state.do_advertise = 0;
	}

	if (ctx->adp_state.do_discover) {
		DEBUG_ADP_PRINTF2("[ADP] do_discover\n");
		adp_advertise_tx_discover(ctx);
		ctx->adp_state.do_discover = 0;
	}

	if (ctx->adp_state.do_terminate) {
		DEBUG_ADP_PRINTF2("[ADP] do_terminate\n");
		adp_advertise_tx_entity_departing(ctx);
		ctx->adp_state.do_terminate = 0;
		ctx->adp_state.do_advertise = 0;
		ctx->adp_state.last_time_in_microseconds =
			current_time_in_microseconds;
		ctx->adpdu.available_index = 0;
	}

	return 0;
}

int adp_state_change_departing(struct adp_state_data *adp_state)
{
	if (!adp_state) {
		DEBUG_ADP_PRINTF1(
			"[ADP] adp_state is NULL in adp_state_change_departing().\n");
		return -1;
	}

	adp_state->do_terminate = 1;

	return 0;
}

int adp_set_grandmaster_id(struct avdecc_ctx *ctx, uint64_t grandmaster_id)
{
	if (!ctx) {
		DEBUG_ADP_PRINTF1(
			"[ADP] ctx is NULL in adp_set_grandmaster_id()\n");
		return -1;
	}

	jdksavdecc_eui64_init_from_uint64(&ctx->adpdu.gptp_grandmaster_id, grandmaster_id);

	return 0;
}
