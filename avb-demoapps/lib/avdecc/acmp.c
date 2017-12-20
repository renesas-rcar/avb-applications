/*
 * Copyright (c) 2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#include <time.h>

#include "avdecc_internal.h"

static struct acmp_listener_stream_info *
acmp_find_listener_stream_info(
	struct configuration_list *configuration_list,
	uint16_t current_configuration,
	uint16_t unique_id)
{
	int i = 0;
	struct configuration_list *config = NULL;

	for (config = configuration_list; config != NULL; config = config->next) {
		if (config->configuration.desc.configuration_desc.configuration.descriptor_index == current_configuration) {
			for (i = 0; i < config->listener_stream_infos_count; i++) {
				if (config->acmp_listener_stream_infos[i].listener_unique_id == unique_id) {
					return &config->acmp_listener_stream_infos[i];
				}
			}
			break;
		}
	}

	return NULL;
}

static int acmp_add_listener_stream_info(
	struct avdecc_ctx *ctx,
	const struct acmp_cmd_resp *p)
{
	struct acmp_listener_stream_info *stream_info;

	stream_info = acmp_find_listener_stream_info(
			ctx->dev_e->configuration_list,
			ctx->dev_e->entity.desc.entity_desc.entity.current_configuration,
			p->listener_unique_id);
	if (!stream_info) {
		/* not found */
		return JDKSAVDECC_ACMP_STATUS_LISTENER_MISBEHAVING;
	}

	if (stream_info->connected) {
		/* already connected */
		return JDKSAVDECC_ACMP_STATUS_LISTENER_EXCLUSIVE;
	}

	jdksavdecc_eui64_copy(&stream_info->talker_entity_id, &p->talker_entity_id);
	jdksavdecc_eui64_copy(&stream_info->stream_id, &p->stream_id);
	jdksavdecc_eui48_copy(&stream_info->stream_dest_mac, &p->stream_dest_mac);
	jdksavdecc_eui64_copy(&stream_info->controller_entity_id, &p->controller_entity_id);
	stream_info->talker_unique_id = p->talker_unique_id;
	stream_info->connected = true;
	stream_info->flags = p->flags;
	stream_info->stream_vlan_id = p->stream_vlan_id;

	return JDKSAVDECC_ACMP_STATUS_SUCCESS;
}

static int acmp_remove_listener_stream_info(
	struct avdecc_ctx *ctx,
	const struct acmp_cmd_resp *p)
{
	int i = 0;

	struct configuration_list *config_list = NULL;
	struct acmp_listener_stream_info *stream_info;

	for (config_list = ctx->dev_e->configuration_list; config_list != NULL; config_list = config_list->next) {
		if (config_list->configuration.desc.configuration_desc.configuration.descriptor_index == ctx->dev_e->entity.desc.entity_desc.entity.current_configuration) {
			for (i = 0; i < config_list->listener_stream_infos_count; i++) {
				stream_info = &config_list->acmp_listener_stream_infos[i];
				if (stream_info->listener_unique_id == p->listener_unique_id) {
					if (stream_info->connected &&
					    (!jdksavdecc_eui64_compare(&stream_info->talker_entity_id, &p->talker_entity_id) &&
					    stream_info->talker_unique_id == p->talker_unique_id)) {
						memset(&config_list->acmp_listener_stream_infos[i], 0, sizeof(struct acmp_listener_stream_info));
						return 0;
					}
				}
			}
			break;
		}
	}

	return -1;
}

static struct acmp_talker_stream_info *
acmp_find_talker_stream_info(
	struct configuration_list *configuration_list,
	uint16_t current_configuration,
	uint16_t unique_id)
{
	int i = 0;
	struct configuration_list *config = NULL;

	for (config = configuration_list; config != NULL; config = config->next) {
		if (config->configuration.desc.configuration_desc.configuration.descriptor_index == current_configuration) {
			for (i = 0; i < config->talker_stream_infos_count; i++) {
				if (config->acmp_talker_stream_infos[i].talker_unique_id == unique_id) {
					return &config->acmp_talker_stream_infos[i];
				}
			}
			break;
		}
	}

	return NULL;
}

struct acmp_listener_pair *acmp_add_new_listener_pair(
	struct acmp_listener_pair *head,
	struct jdksavdecc_eui64 listener_entity_id,
	int listener_unique_id)
{
	struct acmp_listener_pair *p = head;
	struct acmp_listener_pair *new_pair;

	new_pair = (struct acmp_listener_pair *)calloc(1, sizeof(struct acmp_listener_pair));
	if (!new_pair) {
		AVDECC_PRINTF1("could not allocat new_config\n");
		return NULL;
	}

	jdksavdecc_eui64_copy(&new_pair->listener_entity_id, &listener_entity_id);
	new_pair->listener_unique_id = listener_unique_id;

	new_pair->next = NULL;

	if (head == NULL)
		return new_pair;

	while (p->next != NULL)
		p = p->next;
	p->next = new_pair;

	return head;
}

static int acmp_remove_listener_pair(
	struct acmp_listener_pair *head,
	struct jdksavdecc_eui64 listener_entity_id,
	int listener_unique_id)
{
	struct acmp_listener_pair *p = NULL;
	struct acmp_listener_pair *next = NULL;
	struct acmp_listener_pair *prev = NULL;

	if (!head)
		return -1;

	for (p = head; p != NULL; prev = p, p = next) {
		if (!jdksavdecc_eui64_compare(&p->listener_entity_id, &listener_entity_id) &&
		    p->listener_unique_id == listener_unique_id) {
			if (p == head) {
				head = head->next;
			} else if (!p->next) {
				free(p);
			} else {
				prev->next = p->next;
				free(p);
			}
			return 0;
		}
		next = p->next;
	}

	return -1;
}

static int acmp_reset_listener_pair(struct acmp_listener_pair *head)
{
	struct acmp_listener_pair *p = NULL;
	struct acmp_listener_pair *next = NULL;

	if (!head)
		return -1;

	for (p = head; p != NULL; p = next) {
		next = p->next;
		free(p);
	}

	return 0;
}

static int acmp_add_talker_stream_info(
	struct avdecc_ctx *ctx,
	const struct acmp_cmd_resp *p)
{
	struct acmp_talker_stream_info *stream_info;

	stream_info = acmp_find_talker_stream_info(
			ctx->dev_e->configuration_list,
			ctx->dev_e->entity.desc.entity_desc.entity.current_configuration,
			p->talker_unique_id);
	if (!stream_info) {
		/* not found */
		return -1;
	}

	stream_info->connected_listeners = acmp_add_new_listener_pair(
						stream_info->connected_listeners,
						p->listener_entity_id,
						p->listener_unique_id);
	if (!stream_info->connected_listeners)
		return -1;

	stream_info->connection_count++;

	return 0;
}

static int acmp_remove_talker_stream_info(
	struct avdecc_ctx *ctx,
	const struct acmp_cmd_resp *p)
{
	int rc = 0;
	struct acmp_talker_stream_info *stream_info;

	stream_info = acmp_find_talker_stream_info(
			ctx->dev_e->configuration_list,
			ctx->dev_e->entity.desc.entity_desc.entity.current_configuration,
			p->talker_unique_id);
	if (!stream_info) {
		/* not found, do nothing */
		return 0;
	}

	rc = acmp_remove_listener_pair(
		stream_info->connected_listeners,
		p->listener_entity_id,
		p->listener_unique_id);
	if (rc < 0)
		return -1;

	stream_info->connection_count--;

	return 0;
}

static int acmp_valid_stream(
	struct configuration_list *configuration_list,
	uint16_t current_configuration,
	uint16_t descriptor_type,
	uint16_t unique_id)
{
	int i = 0;
	struct configuration_list *config = NULL;
	struct descriptor_data *desc_data = NULL;
	uint8_t stream_infos_count;

	for (config = configuration_list; config != NULL; config = config->next) {
		if (config->configuration.desc.configuration_desc.configuration.descriptor_index == current_configuration) {
			if (descriptor_type == JDKSAVDECC_DESCRIPTOR_STREAM_INPUT)
				stream_infos_count = config->listener_stream_infos_count;
			else
				stream_infos_count = config->talker_stream_infos_count;

			for (i = 0; i < stream_infos_count; i++) {
				for (desc_data = config->descriptor; desc_data != NULL; desc_data = desc_data->next) {
					if (desc_data->desc.stream_desc.stream.descriptor_type == descriptor_type &&
					    desc_data->desc.stream_desc.stream.descriptor_index == unique_id) {
						/* found */
						return 1;
					}
				}
			}
			break;
		}
	}

	return 0;
}

/* IEEE1722.1-2013 Chapter.8.2.2.6.2.1 */
static int acmp_valid_talker_unique(
	struct avdecc_ctx *ctx,
	uint16_t talker_unique_id)
{
	return acmp_valid_stream(
			ctx->dev_e->configuration_list,
			ctx->dev_e->entity.desc.entity_desc.entity.current_configuration,
			JDKSAVDECC_DESCRIPTOR_STREAM_OUTPUT,
			talker_unique_id);
}

/* IEEE1722.1-2013 Chapter.8.2.2.5.2.1 */
static int acmp_valid_listener_unique(
	struct avdecc_ctx *ctx,
	uint16_t listener_unique_id)
{
	return acmp_valid_stream(
			ctx->dev_e->configuration_list,
			ctx->dev_e->entity.desc.entity_desc.entity.current_configuration,
			JDKSAVDECC_DESCRIPTOR_STREAM_INPUT,
			listener_unique_id);
}

/* IEEE1722.1-2013 Chapter.8.2.2.5.2.2 */
static bool acmp_listener_is_connected(
	struct avdecc_ctx *ctx,
	const struct acmp_cmd_resp *p)
{
	struct acmp_listener_stream_info *stream_info;

	stream_info = acmp_find_listener_stream_info(
			ctx->dev_e->configuration_list,
			ctx->dev_e->entity.desc.entity_desc.entity.current_configuration,
			p->listener_unique_id);
	if (!stream_info) {
		/* not found */
		return false;
	}

	if (stream_info->connected &&
	    (jdksavdecc_eui64_compare(&stream_info->talker_entity_id, &p->talker_entity_id) &&
	    stream_info->talker_unique_id != p->talker_unique_id)) {
		return true;
	} else {
		return false;
	}
}

/* IEEE1722.1-2013 Chapter.8.2.2.5.2.3 */
static int acmp_listener_is_connectedto(
	struct avdecc_ctx *ctx,
	const struct acmp_cmd_resp *p)
{
	struct acmp_listener_stream_info *stream_info;

	stream_info = acmp_find_listener_stream_info(
			ctx->dev_e->configuration_list,
			ctx->dev_e->entity.desc.entity_desc.entity.current_configuration,
			p->listener_unique_id);
	if (!stream_info) {
		/* not found */
		return true;
	}

	if (!stream_info->connected ||
	    (jdksavdecc_eui64_compare(&stream_info->talker_entity_id, &p->talker_entity_id) ||
	    stream_info->talker_unique_id != p->talker_unique_id)) {
		return false;
	} else {
		return true;
	}
}

/* IEEE1722.1-2013 Chapter.8.2.2.5.2.5 */
static int acmp_tx_response(
	uint8_t message_type,
	const struct acmp_cmd_resp *acmp_cmd_resp,
	uint8_t status,
	struct avdecc_ctx *ctx)
{
	struct jdksavdecc_frame frame;
	struct jdksavdecc_acmpdu acmpdu;

	acmpdu.header.cd = 1;
	acmpdu.header.subtype = JDKSAVDECC_SUBTYPE_ACMP;
	acmpdu.header.sv = 0;
	acmpdu.header.version = 0;
	acmpdu.header.message_type = message_type;
	acmpdu.header.status = status;
	acmpdu.header.control_data_length =
		JDKSAVDECC_ACMPDU_LEN - JDKSAVDECC_COMMON_CONTROL_HEADER_LEN;

	jdksavdecc_eui64_copy(&acmpdu.header.stream_id, &acmp_cmd_resp->stream_id);
	jdksavdecc_eui64_copy(&acmpdu.controller_entity_id, &acmp_cmd_resp->controller_entity_id);
	jdksavdecc_eui64_copy(&acmpdu.talker_entity_id, &acmp_cmd_resp->talker_entity_id);
	jdksavdecc_eui64_copy(&acmpdu.listener_entity_id, &acmp_cmd_resp->listener_entity_id);

	acmpdu.talker_unique_id = acmp_cmd_resp->talker_unique_id;
	acmpdu.listener_unique_id = acmp_cmd_resp->listener_unique_id;
	jdksavdecc_eui48_copy(&acmpdu.stream_dest_mac, &acmp_cmd_resp->stream_dest_mac);
	acmpdu.connection_count = acmp_cmd_resp->connection_count;
	acmpdu.sequence_id = acmp_cmd_resp->sequence_id;
	acmpdu.flags = acmp_cmd_resp->flags;
	acmpdu.stream_vlan_id = acmp_cmd_resp->stream_vlan_id;
	acmpdu.reserved = 0x0000;

	frame.length = jdksavdecc_acmpdu_write(&acmpdu,
						&frame.payload,
						0,
						sizeof(frame.payload));

	frame.dest_address = jdksavdecc_multicast_adp_acmp;

	if (frame.length > 0) {
		if (send_data(&ctx->net_info,
			      frame.dest_address.value,
			      frame.payload,
			      frame.length) > 0) {
			DEBUG_ACMP_PRINTF2("[ACMP] Sent: ACMPDU RESPONSE\n");
		}
	}

	return 0;
}

/* IEEE1722.1-2013 Chapter.8.2.2.5.2.4 */
static int acmp_tx_command(
	uint8_t message_type,
	const struct acmp_cmd_resp *acmp_cmd_resp,
	struct acmp_inflight_command *inflight,
	struct avdecc_ctx *ctx)
{
	struct timespec t;
	int i = 0;
	struct jdksavdecc_frame frame;
	struct jdksavdecc_acmpdu acmpdu;
	int ms_timeout_priod;

	acmpdu.header.cd = 1;
	acmpdu.header.subtype = JDKSAVDECC_SUBTYPE_ACMP;
	acmpdu.header.sv = 0;
	acmpdu.header.version = 0;
	acmpdu.header.message_type = message_type;
	acmpdu.header.status = JDKSAVDECC_ACMP_STATUS_SUCCESS;
	acmpdu.header.control_data_length =
		JDKSAVDECC_ACMPDU_LEN - JDKSAVDECC_COMMON_CONTROL_HEADER_LEN;

	jdksavdecc_eui64_copy(&acmpdu.header.stream_id,
			      &acmp_cmd_resp->stream_id);
	jdksavdecc_eui64_copy(&acmpdu.controller_entity_id,
			      &acmp_cmd_resp->controller_entity_id);
	jdksavdecc_eui64_copy(&acmpdu.talker_entity_id,
			      &acmp_cmd_resp->talker_entity_id);
	jdksavdecc_eui64_copy(&acmpdu.listener_entity_id,
			      &acmp_cmd_resp->listener_entity_id);
	acmpdu.talker_unique_id   = acmp_cmd_resp->talker_unique_id;
	acmpdu.listener_unique_id = acmp_cmd_resp->listener_unique_id;
	jdksavdecc_eui48_copy(&acmpdu.stream_dest_mac,
			      &acmp_cmd_resp->stream_dest_mac);
	acmpdu.connection_count   = acmp_cmd_resp->connection_count;
	acmpdu.sequence_id        = acmp_cmd_resp->sequence_id;
	acmpdu.flags              = acmp_cmd_resp->flags;
	acmpdu.stream_vlan_id     = acmp_cmd_resp->stream_vlan_id;

	frame.length = jdksavdecc_acmpdu_write(&acmpdu,
					       &frame.payload,
					       0,
					       sizeof(frame.payload));

	frame.dest_address = jdksavdecc_multicast_adp_acmp;

	clock_gettime(CLOCK_MONOTONIC, &t);

	if (!inflight) {
		for (i = 0; i < MAX_INFLIGHT_CMD_NUM; i++) {
			inflight = &(ctx->dev_e->acmp_listener_inflight_commands[i]);
			if (inflight->timeout != 0) {
				continue;
			} else {
				inflight->command.message_type       = acmpdu.header.message_type;
				inflight->command.status             = acmpdu.header.status;
				jdksavdecc_eui64_copy(&inflight->command.stream_id, &acmpdu.header.stream_id);
				jdksavdecc_eui64_copy(&inflight->command.controller_entity_id, &acmpdu.controller_entity_id);
				jdksavdecc_eui64_copy(&inflight->command.talker_entity_id, &acmpdu.talker_entity_id);
				jdksavdecc_eui64_copy(&inflight->command.listener_entity_id, &acmpdu.listener_entity_id);
				inflight->command.talker_unique_id   = acmpdu.talker_unique_id;
				inflight->command.listener_unique_id = acmpdu.listener_unique_id;
				jdksavdecc_eui48_copy(&inflight->command.stream_dest_mac, &acmpdu.stream_dest_mac);
				inflight->command.connection_count   = acmpdu.connection_count;
				inflight->command.sequence_id        = acmpdu.sequence_id;
				inflight->command.flags              = acmpdu.flags;
				inflight->command.stream_vlan_id     = acmpdu.stream_vlan_id;

				inflight->retried  = false;
				inflight->original_sequence_id  = acmp_cmd_resp->sequence_id;
				break;
			}
		}
		if (i == MAX_INFLIGHT_CMD_NUM) {
			DEBUG_ACMP_PRINTF1("could not add ctx->dev_e->acmp_listener_inflight_commands[%d] is full\n", MAX_INFLIGHT_CMD_NUM);
			acmp_tx_response(message_type, acmp_cmd_resp, JDKSAVDECC_ACMP_STATUS_LISTENER_MISBEHAVING, ctx);
			return -1;
		}
	}

	switch (inflight->command.message_type) {
	case JDKSAVDECC_ACMP_MESSAGE_TYPE_CONNECT_TX_COMMAND:
		ms_timeout_priod = JDKSAVDECC_ACMP_TIMEOUT_CONNECT_TX_COMMAND_MS;
		break;
	case JDKSAVDECC_ACMP_MESSAGE_TYPE_DISCONNECT_TX_COMMAND:
		ms_timeout_priod = JDKSAVDECC_ACMP_TIMEOUT_DISCONNECT_TX_COMMAND_MS;
		break;
	default:
		ms_timeout_priod = 0;
		break;
	}

	inflight->timeout = (t.tv_sec * 1000000) + (t.tv_nsec / 1000) + (ms_timeout_priod * 1000);

	if (frame.length > 0) {
		if (send_data(&ctx->net_info,
			      frame.dest_address.value,
			      frame.payload,
			      frame.length) > 0) {
			DEBUG_ACMP_PRINTF2("[ACMP] Sent: ACMPDU RESPONSE\n");
		} else {
			acmp_tx_response(message_type, acmp_cmd_resp, JDKSAVDECC_ACMP_STATUS_COULD_NOT_SEND_MESSAGE, ctx);
			return -1;
		}
	}

	return 0;
}

/* IEEE1722.1-2013 Figure8.3 - CONNECT RX COMMAND */
static int acmp_process_connect_rx_cmd(
	struct avdecc_ctx *ctx,
	const struct acmp_cmd_resp *acmp_cmd_resp)
{
	if (acmp_valid_listener_unique(ctx, acmp_cmd_resp->listener_unique_id)) {
		if (!acmp_listener_is_connected(ctx, acmp_cmd_resp)) {
			acmp_tx_command(JDKSAVDECC_ACMP_MESSAGE_TYPE_CONNECT_TX_COMMAND,
					acmp_cmd_resp,
					NULL,
					ctx);
		} else {
			acmp_tx_response(JDKSAVDECC_ACMP_MESSAGE_TYPE_CONNECT_RX_RESPONSE,
					 acmp_cmd_resp,
					 JDKSAVDECC_ACMP_STATUS_LISTENER_EXCLUSIVE,
					 ctx);
		}
	} else {
		acmp_tx_response(JDKSAVDECC_ACMP_MESSAGE_TYPE_CONNECT_RX_RESPONSE,
				 acmp_cmd_resp,
				 JDKSAVDECC_ACMP_STATUS_LISTENER_UNKNOWN_ID,
				 ctx);
	}

	return 0;
}

/* IEEE1722.1-2013 Figure8.3 - CONNECT TX RESPONSE */
static int acmp_process_connect_tx_response(
	struct avdecc_ctx *ctx,
	const struct acmp_cmd_resp *acmp_cmd_resp)
{
	int i = 0;
	struct acmp_cmd_resp response;
	struct acmp_inflight_command *inflight;

	if (acmp_valid_listener_unique(ctx, acmp_cmd_resp->listener_unique_id)) {
		memcpy(&response, acmp_cmd_resp, sizeof(struct acmp_cmd_resp));

		if (acmp_cmd_resp->status == JDKSAVDECC_ACMP_STATUS_SUCCESS) {
			for (i = 0; i < MAX_INFLIGHT_CMD_NUM; i++) {
				inflight = &(ctx->dev_e->acmp_listener_inflight_commands[i]);
				if (inflight->timeout &&
				    inflight->command.message_type == JDKSAVDECC_ACMP_MESSAGE_TYPE_CONNECT_TX_COMMAND &&
				    inflight->command.sequence_id == acmp_cmd_resp->sequence_id) {
					/* connectListener */
					response.status = acmp_add_listener_stream_info(ctx, acmp_cmd_resp);
					response.sequence_id = inflight->original_sequence_id;
					response.connection_count = acmp_cmd_resp->connection_count;
					/* cancelTimeout() and removeInflight()*/
					memset(inflight, 0, sizeof(struct acmp_inflight_command));
					break;
				}
			}
			if (i == MAX_INFLIGHT_CMD_NUM)
				response.status = JDKSAVDECC_ACMP_STATUS_LISTENER_MISBEHAVING;
		}
		acmp_tx_response(JDKSAVDECC_ACMP_MESSAGE_TYPE_CONNECT_RX_RESPONSE, &response, response.status, ctx);
	}

	return 0;
}

/* IEEE1722.1-2013 Figure8.3 - GET RX STATE */
static int acmp_process_get_rx_state_cmd(
	struct avdecc_ctx *ctx,
	const struct acmp_cmd_resp *acmp_cmd_resp)
{
	int error = JDKSAVDECC_ACMP_STATUS_SUCCESS;
	struct acmp_cmd_resp response;
	struct acmp_listener_stream_info *stream_info;

	memcpy(&response, acmp_cmd_resp, sizeof(struct acmp_cmd_resp));

	if (acmp_valid_listener_unique(ctx, acmp_cmd_resp->listener_unique_id)) {
		/* getState() */
		stream_info = acmp_find_listener_stream_info(
			ctx->dev_e->configuration_list,
			ctx->dev_e->entity.desc.entity_desc.entity.current_configuration,
			acmp_cmd_resp->listener_unique_id);
		if (!stream_info) {
			error = JDKSAVDECC_ACMP_STATUS_LISTENER_MISBEHAVING;
		} else {
			response.connection_count = stream_info->connected;
			response.flags = stream_info->flags;
			response.stream_vlan_id = stream_info->stream_vlan_id;
			jdksavdecc_eui64_copy(&response.stream_id, &stream_info->stream_id);
			jdksavdecc_eui48_copy(&response.stream_dest_mac, &stream_info->stream_dest_mac);
			jdksavdecc_eui64_copy(&response.talker_entity_id, &stream_info->talker_entity_id);

			error = JDKSAVDECC_ACMP_STATUS_SUCCESS;
		}
	} else {
		error = JDKSAVDECC_ACMP_STATUS_LISTENER_UNKNOWN_ID;
	}

	acmp_tx_response(JDKSAVDECC_ACMP_MESSAGE_TYPE_GET_RX_STATE_RESPONSE, &response, error, ctx);

	return 0;
}

/* IEEE1722.1-2013 Figure8.3 - DISCONNECT RX COMMAND */
static int acmp_process_disconnect_rx_cmd(
	struct avdecc_ctx *ctx,
	const struct acmp_cmd_resp *acmp_cmd_resp)
{
	if (acmp_valid_listener_unique(ctx, acmp_cmd_resp->listener_unique_id)) {
		if (acmp_listener_is_connectedto(ctx, acmp_cmd_resp)) {
			acmp_tx_command(JDKSAVDECC_ACMP_MESSAGE_TYPE_DISCONNECT_TX_COMMAND,
					acmp_cmd_resp,
					NULL,
					ctx);
		} else {
			acmp_tx_response(JDKSAVDECC_ACMP_MESSAGE_TYPE_DISCONNECT_RX_RESPONSE,
					 acmp_cmd_resp,
					 JDKSAVDECC_ACMP_STATUS_NOT_CONNECTED,
					 ctx);
		}
	} else {
		acmp_tx_response(JDKSAVDECC_ACMP_MESSAGE_TYPE_DISCONNECT_RX_RESPONSE,
				 acmp_cmd_resp,
				 JDKSAVDECC_ACMP_STATUS_LISTENER_UNKNOWN_ID,
				 ctx);
	}

	return 0;
}

/* IEEE1722.1-2013 Figure8.3 - DISCONNECT TX RESPONSE */
static int acmp_process_disconnect_tx_response(
	struct avdecc_ctx *ctx,
	const struct acmp_cmd_resp *acmp_cmd_resp)
{
	int rc = 0;
	struct acmp_cmd_resp response;
	struct acmp_inflight_command *inflight;

	if (acmp_valid_listener_unique(ctx, acmp_cmd_resp->listener_unique_id)) {
		memcpy(&response, acmp_cmd_resp, sizeof(struct acmp_cmd_resp));
		for (int i = 0; i < MAX_INFLIGHT_CMD_NUM; i++) {
			inflight = &(ctx->dev_e->acmp_listener_inflight_commands[i]);
			if (inflight->timeout &&
			    inflight->command.sequence_id == acmp_cmd_resp->sequence_id) {
				response.sequence_id = inflight->original_sequence_id;
				rc = acmp_remove_listener_stream_info(ctx, acmp_cmd_resp);
				if (rc < 0) {
					DEBUG_ACMP_PRINTF1("[ACMP] Could not disconnect stream\n");
					response.status = JDKSAVDECC_ACMP_STATUS_LISTENER_MISBEHAVING;
				} else {
					memset(inflight, 0, sizeof(struct acmp_inflight_command));
					acmp_tx_response(JDKSAVDECC_ACMP_MESSAGE_TYPE_DISCONNECT_RX_RESPONSE, &response, response.status, ctx);
				}
				break;
			}
		}
	}

	return 0;
}

/* IEEE1722.1-2013 Figure8.3 - CONNECT TX TIMEOUT */
static int acmp_process_connect_tx_timeout(
	struct avdecc_ctx *ctx,
	struct acmp_inflight_command *inflight)
{
	struct acmp_cmd_resp response;

	if (inflight->retried) {
		memcpy(&response, &inflight->command, sizeof(struct acmp_cmd_resp));
		response.sequence_id = inflight->original_sequence_id;
		acmp_tx_response(JDKSAVDECC_ACMP_MESSAGE_TYPE_CONNECT_RX_RESPONSE, &response, JDKSAVDECC_ACMP_STATUS_LISTENER_TALKER_TIMEOUT, ctx);
		memset(inflight, 0, sizeof(struct acmp_inflight_command));
	} else {
		acmp_tx_command(JDKSAVDECC_ACMP_MESSAGE_TYPE_CONNECT_TX_COMMAND, &inflight->command, inflight, ctx);
		inflight->retried  = true;
	}

	return 0;
}

/* IEEE1722.1-2013 Figure8.3 - DISCONNECT TX TIMEOUT */
static int acmp_process_disconnect_tx_timeout(
	struct avdecc_ctx *ctx,
	struct acmp_inflight_command *inflight)
{
	struct acmp_cmd_resp response;

	if (inflight->retried) {
		memcpy(&response, &inflight->command, sizeof(struct acmp_cmd_resp));
		response.sequence_id = inflight->original_sequence_id;
		acmp_tx_response(JDKSAVDECC_ACMP_MESSAGE_TYPE_DISCONNECT_RX_RESPONSE, &response, JDKSAVDECC_ACMP_STATUS_LISTENER_TALKER_TIMEOUT, ctx);
		memset(inflight, 0, sizeof(struct acmp_inflight_command));
	} else {
		acmp_tx_command(JDKSAVDECC_ACMP_MESSAGE_TYPE_DISCONNECT_TX_COMMAND, &inflight->command, inflight, ctx);
		inflight->retried  = true;
	}

	return 0;
}

/* IEEE1722.1-2013 Figure8.3 */
static int acmp_process_listener_state_machine(
	struct avdecc_ctx *ctx,
	const struct acmp_cmd_resp *acmp_cmd_resp)
{
	switch (acmp_cmd_resp->message_type) {
	/* ACMP Listener state machine*/
	case JDKSAVDECC_ACMP_MESSAGE_TYPE_CONNECT_RX_COMMAND:
		return acmp_process_connect_rx_cmd(ctx, acmp_cmd_resp);
	case JDKSAVDECC_ACMP_MESSAGE_TYPE_CONNECT_TX_RESPONSE:
		return acmp_process_connect_tx_response(ctx, acmp_cmd_resp);
	case JDKSAVDECC_ACMP_MESSAGE_TYPE_GET_RX_STATE_COMMAND:
		return acmp_process_get_rx_state_cmd(ctx, acmp_cmd_resp);
	case JDKSAVDECC_ACMP_MESSAGE_TYPE_DISCONNECT_RX_COMMAND:
		return acmp_process_disconnect_rx_cmd(ctx, acmp_cmd_resp);
	case JDKSAVDECC_ACMP_MESSAGE_TYPE_DISCONNECT_TX_RESPONSE:
		return acmp_process_disconnect_tx_response(ctx, acmp_cmd_resp);
	default:
		return -1; /* do nothing */
	}
}

/* IEEE1722.1-2013 Chapter.8.2.2.6.2.2 */
static int acmp_connect_talker(
	struct avdecc_ctx *ctx,
	uint16_t current_configuration,
	struct acmp_cmd_resp *response,
	const struct acmp_cmd_resp *acmp_cmd_resp)
{
	int rc;
	int error;
	struct acmp_talker_stream_info *stream_info;

	stream_info = acmp_find_talker_stream_info(
			ctx->dev_e->configuration_list,
			ctx->dev_e->entity.desc.entity_desc.entity.current_configuration,
			acmp_cmd_resp->talker_unique_id);
	if (!stream_info) {
		/* not found */
		return JDKSAVDECC_ACMP_STATUS_TALKER_MISBEHAVING;
	}

	rc = acmp_add_talker_stream_info(ctx, acmp_cmd_resp);
	if (rc < 0)
		error = JDKSAVDECC_ACMP_STATUS_TALKER_MISBEHAVING;
	else
		error = JDKSAVDECC_ACMP_STATUS_SUCCESS;

	jdksavdecc_eui64_copy(&response->stream_id, &stream_info->stream_id);
	jdksavdecc_eui48_copy(&response->stream_dest_mac, &stream_info->stream_dest_mac);
	response->stream_vlan_id = stream_info->stream_vlan_id;
	response->connection_count = stream_info->connection_count;

	return error;
}

/* IEEE1722.1-2013 Figure8.4 - CONNECT */
static int acmp_process_connect_tx_cmd(
	struct avdecc_ctx *ctx,
	const struct acmp_cmd_resp *acmp_cmd_resp)
{
	int error = JDKSAVDECC_ACMP_STATUS_SUCCESS;
	struct acmp_cmd_resp response;

	memcpy(&response, acmp_cmd_resp, sizeof(struct acmp_cmd_resp));

	if (acmp_valid_talker_unique(ctx, acmp_cmd_resp->talker_unique_id)) {
		/* connectTalker() */
		error = acmp_connect_talker(ctx,
					    ctx->dev_e->entity.desc.entity_desc.entity.current_configuration,
					    &response,
					    acmp_cmd_resp);
	} else {
		error = JDKSAVDECC_ACMP_STATUS_TALKER_UNKNOWN_ID;
	}

	acmp_tx_response(JDKSAVDECC_ACMP_MESSAGE_TYPE_CONNECT_TX_RESPONSE,
			 &response,
			 error,
			 ctx);

	return 0;
}

/* IEEE1722.1-2013 Figure8.4 - DISCONNECT */
static int acmp_process_disconnect_tx_cmd(
	struct avdecc_ctx *ctx,
	const struct acmp_cmd_resp *acmp_cmd_resp)
{
	int error = JDKSAVDECC_ACMP_STATUS_SUCCESS;

	if (acmp_valid_talker_unique(ctx, acmp_cmd_resp->talker_unique_id)) {
		/* disconnectTalker() */
		acmp_remove_talker_stream_info(ctx, acmp_cmd_resp);
		error = JDKSAVDECC_ACMP_STATUS_SUCCESS;
	} else {
		error = JDKSAVDECC_ACMP_STATUS_TALKER_UNKNOWN_ID;
	}

	acmp_tx_response(JDKSAVDECC_ACMP_MESSAGE_TYPE_DISCONNECT_TX_RESPONSE,
			 acmp_cmd_resp,
			 error,
			 ctx);

	return 0;
}

/* IEEE1722.1-2013 Figure8.4 - GET STATE */
static int acmp_process_get_tx_state_cmd(
	struct avdecc_ctx *ctx,
	const struct acmp_cmd_resp *acmp_cmd_resp)
{
	int error = JDKSAVDECC_ACMP_STATUS_SUCCESS;
	struct acmp_cmd_resp response;
	struct acmp_talker_stream_info *stream_info;

	memcpy(&response, acmp_cmd_resp, sizeof(struct acmp_cmd_resp));

	if (acmp_valid_talker_unique(ctx, acmp_cmd_resp->talker_unique_id)) {
		/* getState() */
		stream_info = acmp_find_talker_stream_info(
				ctx->dev_e->configuration_list,
				ctx->dev_e->entity.desc.entity_desc.entity.current_configuration,
				acmp_cmd_resp->talker_unique_id);
		if (!stream_info) {
			error = JDKSAVDECC_ACMP_STATUS_TALKER_MISBEHAVING;
		} else {
			response.connection_count = stream_info->connection_count;
			jdksavdecc_eui64_copy(&response.stream_id, &stream_info->stream_id);
			jdksavdecc_eui48_copy(&response.stream_dest_mac, &stream_info->stream_dest_mac);
			response.stream_vlan_id = stream_info->stream_vlan_id;

			error = JDKSAVDECC_ACMP_STATUS_SUCCESS;
		}
	} else {
		error = JDKSAVDECC_ACMP_STATUS_TALKER_UNKNOWN_ID;
	}

	acmp_tx_response(JDKSAVDECC_ACMP_MESSAGE_TYPE_GET_TX_STATE_RESPONSE,
			 &response,
			 error,
			 ctx);

	return 0;
}

/* IEEE1722.1-2013 Figure8.4 - GET CONNECTION */
static int acmp_process_get_tx_connection_cmd(
	struct avdecc_ctx *ctx,
	const struct acmp_cmd_resp *acmp_cmd_resp)
{
	int i = 0;
	int error = JDKSAVDECC_ACMP_STATUS_SUCCESS;
	struct acmp_cmd_resp response;
	struct acmp_talker_stream_info *stream_info;
	struct acmp_listener_pair *tmp_pair;

	memcpy(&response, acmp_cmd_resp, sizeof(struct acmp_cmd_resp));

	if (!acmp_valid_talker_unique(ctx, acmp_cmd_resp->talker_unique_id)) {
		error = JDKSAVDECC_ACMP_STATUS_TALKER_UNKNOWN_ID;
	} else {
		/* getConnection() */
		stream_info = acmp_find_talker_stream_info(
				ctx->dev_e->configuration_list,
				ctx->dev_e->entity.desc.entity_desc.entity.current_configuration,
				acmp_cmd_resp->talker_unique_id);
		if (!stream_info) {
			error = JDKSAVDECC_ACMP_STATUS_TALKER_MISBEHAVING;
		} else {
			tmp_pair = stream_info->connected_listeners;
			if (!tmp_pair) {
				error = JDKSAVDECC_ACMP_STATUS_NO_SUCH_CONNECTION;
			} else {
				if (acmp_cmd_resp->connection_count) {
					while (i < acmp_cmd_resp->connection_count) {
						if (!tmp_pair->next)
							error = JDKSAVDECC_ACMP_STATUS_NO_SUCH_CONNECTION;
						else
							tmp_pair = tmp_pair->next;
					}
				}

				if (error == JDKSAVDECC_ACMP_STATUS_SUCCESS) {
					jdksavdecc_eui64_copy(&response.listener_entity_id, &tmp_pair->listener_entity_id);
					response.listener_unique_id = tmp_pair->listener_unique_id;
					response.connection_count = stream_info->connection_count;
					jdksavdecc_eui64_copy(&response.stream_id, &stream_info->stream_id);
					jdksavdecc_eui48_copy(&response.stream_dest_mac, &stream_info->stream_dest_mac);
					response.stream_vlan_id = stream_info->stream_vlan_id;
				}
			}
		}
	}

	acmp_tx_response(JDKSAVDECC_ACMP_MESSAGE_TYPE_GET_TX_CONNECTION_RESPONSE, &response, error, ctx);

	return 0;
}

/* IEEE1722.1-2013 Figure8.4 */
static int acmp_process_talker_state_machine(
	struct avdecc_ctx *ctx,
	const struct acmp_cmd_resp *acmp_cmd_resp)
{
	switch (acmp_cmd_resp->message_type) {
	/* ACMP Talker state machine*/
	case JDKSAVDECC_ACMP_MESSAGE_TYPE_CONNECT_TX_COMMAND:
		return acmp_process_connect_tx_cmd(ctx, acmp_cmd_resp);
	case JDKSAVDECC_ACMP_MESSAGE_TYPE_DISCONNECT_TX_COMMAND:
		return acmp_process_disconnect_tx_cmd(ctx, acmp_cmd_resp);
	case JDKSAVDECC_ACMP_MESSAGE_TYPE_GET_TX_STATE_COMMAND:
		return acmp_process_get_tx_state_cmd(ctx, acmp_cmd_resp);
	case JDKSAVDECC_ACMP_MESSAGE_TYPE_GET_TX_CONNECTION_COMMAND:
		return acmp_process_get_tx_connection_cmd(ctx, acmp_cmd_resp);
	default:
		return -1; /* do nothing */
	}
}

/*
 * public functions
 */
int acmp_ctx_init(struct avdecc_ctx *ctx)
{
	int input_count = 0;
	int output_count = 0;

	struct configuration_list *config_list = NULL;
	struct descriptor_data *desc_data = NULL;

	if (!ctx) {
		DEBUG_ACMP_PRINTF1("[ACMP] ctx is NULL in acmp_ctx_init().\n");
		return -1;
	}

	for (config_list = ctx->dev_e->configuration_list; config_list != NULL; config_list = config_list->next) {
		/* counting stream input and output */
		for (desc_data = config_list->descriptor; desc_data != NULL; desc_data = desc_data->next) {
			if (desc_data->desc.stream_desc.stream.descriptor_type == JDKSAVDECC_DESCRIPTOR_STREAM_INPUT)
				input_count++;
			if (desc_data->desc.stream_desc.stream.descriptor_type == JDKSAVDECC_DESCRIPTOR_STREAM_OUTPUT)
				output_count++;
		}

		/* allocate listener stream info and fill unique_id */
		config_list->acmp_listener_stream_infos = calloc(input_count, sizeof(struct acmp_listener_stream_info));
		config_list->listener_stream_infos_count = input_count;

		input_count = 0;
		for (desc_data = config_list->descriptor; desc_data != NULL; desc_data = desc_data->next) {
			if (desc_data->desc.stream_desc.stream.descriptor_type == JDKSAVDECC_DESCRIPTOR_STREAM_INPUT) {
				config_list->acmp_listener_stream_infos[input_count].listener_unique_id =
					desc_data->desc.stream_desc.stream.descriptor_index;
				input_count++;
			}
		}

		/* allocate talker stream info and fill unique_id */
		config_list->acmp_talker_stream_infos = calloc(output_count, sizeof(struct acmp_talker_stream_info));
		config_list->talker_stream_infos_count = output_count;

		output_count = 0;
		for (desc_data = config_list->descriptor; desc_data != NULL; desc_data = desc_data->next) {
			if (desc_data->desc.stream_desc.stream.descriptor_type == JDKSAVDECC_DESCRIPTOR_STREAM_OUTPUT) {
				config_list->acmp_talker_stream_infos[output_count].talker_unique_id =
					desc_data->desc.stream_desc.stream.descriptor_index;
				output_count++;
			}
		}
	}

	return 0;
}

int acmp_set_streamid(
	struct configuration_list *configuration_list,
	uint16_t current_configuration,
	uint16_t unique_id,
	uint64_t stream_id)
{
	struct acmp_talker_stream_info *stream_info;

	if (!configuration_list) {
		DEBUG_ACMP_PRINTF1("[ACMP] configuration_list is NULL in acmp_set_streamid().\n");
		return -1;
	}

	stream_info = acmp_find_talker_stream_info(
		configuration_list,
		current_configuration,
		unique_id);
	if (!stream_info) {
		/* not found */
		return -1;
	}

	jdksavdecc_eui64_init_from_uint64(&stream_info->stream_id, stream_id);

	return 0;
}

int acmp_set_stream_dest_mac(
	struct configuration_list *configuration_list,
	uint16_t current_configuration,
	uint16_t unique_id,
	uint64_t stream_dest_mac)
{
	struct acmp_talker_stream_info *stream_info;

	if (!configuration_list) {
		DEBUG_ACMP_PRINTF1("[ACMP] configuration_list is NULL in acmp_set_stream_dest_mac().\n");
		return -1;
	}

	stream_info = acmp_find_talker_stream_info(
		configuration_list,
		current_configuration,
		unique_id);
	if (!stream_info) {
		/* not found */
		return -1;
	}

	jdksavdecc_eui48_init_from_uint64(&stream_info->stream_dest_mac, stream_dest_mac);

	return 0;
}

int acmp_set_stream_vlan_id(
	struct configuration_list *configuration_list,
	uint16_t current_configuration,
	uint16_t unique_id,
	uint16_t stream_vlan_id)
{
	struct acmp_talker_stream_info *stream_info;

	if (!configuration_list) {
		DEBUG_ACMP_PRINTF1("[ACMP] configuration_list is NULL in acmp_set_stream_vlan_id().\n");
		return -1;
	}

	stream_info = acmp_find_talker_stream_info(
		configuration_list,
		current_configuration,
		unique_id);
	if (!stream_info) {
		/* not found */
		return -1;
	}

	stream_info->stream_vlan_id = stream_vlan_id;

	return 0;
}

int acmp_get_connection_count(
	struct configuration_list *configuration_list,
	uint16_t current_configuration,
	uint16_t unique_id)
{
	struct acmp_talker_stream_info *stream_info;

	if (!configuration_list) {
		DEBUG_ACMP_PRINTF1("[ACMP] configuration_list is NULL in acmp_get_connection_count().\n");
		return -1;
	}

	stream_info = acmp_find_talker_stream_info(
		configuration_list,
		current_configuration,
		unique_id);
	if (!stream_info) {
		/* not found */
		return -1;
	}

	return stream_info->connection_count;
}

int acmp_get_connected_from_listener_stream_info(
	struct configuration_list *configuration_list,
	uint16_t current_configuration,
	uint16_t unique_id)
{
	struct acmp_listener_stream_info *stream_info;

	if (!configuration_list) {
		DEBUG_ACMP_PRINTF1("[ACMP] configuration_list is NULL in acmp_get_connected_from_listener_stream_info().\n");
		return -1;
	}

	stream_info = acmp_find_listener_stream_info(
		configuration_list,
		current_configuration,
		unique_id);
	if (!stream_info) {
		/* not found */
		return -1;
	}

	return stream_info->connected;
}

int acmp_get_stream_id_from_listener_stream_info(
	struct configuration_list *configuration_list,
	uint16_t current_configuration,
	uint16_t unique_id,
	uint8_t *stream_id)
{
	struct acmp_listener_stream_info *stream_info;

	if (!configuration_list) {
		DEBUG_ACMP_PRINTF1("[ACMP] configuration_list is NULL in acmp_get_stream_id_from_listener_stream_info().\n");
		return -1;
	}

	if (!stream_id) {
		DEBUG_ACMP_PRINTF1("[ACMP] stream_id is NULL in acmp_get_stream_id_from_listener_stream_info().\n");
		return -1;
	}

	stream_info = acmp_find_listener_stream_info(
		configuration_list,
		current_configuration,
		unique_id);
	if (!stream_info) {
		/* not found */
		return -1;
	}

	jdksavdecc_eui64_set(stream_info->stream_id, stream_id, 0);

	return 0;
}

int acmp_ctx_terminate(struct configuration_list *configuration_list)
{
	if (!configuration_list) {
		DEBUG_ACMP_PRINTF1("[ACMP] configuration_list is NULL in acmp_ctx_terminate().\n");
		return -1;
	}

	for (; configuration_list != NULL; configuration_list = configuration_list->next) {
		if (configuration_list->acmp_listener_stream_infos)
			free(configuration_list->acmp_listener_stream_infos);

		if (configuration_list->acmp_talker_stream_infos) {
			acmp_reset_listener_pair(configuration_list->acmp_talker_stream_infos->connected_listeners);
			free(configuration_list->acmp_talker_stream_infos);
		}
	}

	return 0;
}

int acmp_receive_process(
	struct avdecc_ctx *ctx,
	const struct jdksavdecc_frame *frame)
{
	struct jdksavdecc_acmpdu acmpdu;
	struct acmp_cmd_resp acmp_cmd_resp;

	if (!ctx) {
		DEBUG_ACMP_PRINTF1("[ACMP] ctx is NULL in acmp_receive_process().\n");
		return -1;
	}

	if (!frame) {
		DEBUG_ACMP_PRINTF1("[ACMP] frame is NULL in acmp_receive_process().\n");
		return -1;
	}

	if (jdksavdecc_acmpdu_read(&acmpdu,
				   frame->payload,
				   0,
				   frame->length) > 0) {
		acmp_cmd_resp.message_type = acmpdu.header.message_type;
		acmp_cmd_resp.status       = acmpdu.header.status;
		jdksavdecc_eui64_copy(&acmp_cmd_resp.stream_id,
				      &acmpdu.header.stream_id);
		jdksavdecc_eui64_copy(&acmp_cmd_resp.controller_entity_id,
				      &acmpdu.controller_entity_id);
		jdksavdecc_eui64_copy(&acmp_cmd_resp.talker_entity_id,
				      &acmpdu.talker_entity_id);
		jdksavdecc_eui64_copy(&acmp_cmd_resp.listener_entity_id,
				      &acmpdu.listener_entity_id);
		acmp_cmd_resp.talker_unique_id   = acmpdu.talker_unique_id;
		acmp_cmd_resp.listener_unique_id = acmpdu.listener_unique_id;
		jdksavdecc_eui48_copy(&acmp_cmd_resp.stream_dest_mac,
				      &acmpdu.stream_dest_mac);
		acmp_cmd_resp.connection_count   = acmpdu.connection_count;
		acmp_cmd_resp.sequence_id        = acmpdu.sequence_id;
		acmp_cmd_resp.flags              = acmpdu.flags;
		acmp_cmd_resp.stream_vlan_id     = acmpdu.stream_vlan_id;

#if DEBUG_ACMP
		{
		struct jdksavdecc_printer p;
		char buf[256];
		jdksavdecc_printer_init(&p, buf, sizeof(buf));
		jdksavdecc_printer_print_eui64(&p, acmp_cmd_resp.talker_entity_id);
		DEBUG_ACMP_PRINTF2("[ACMP] talker_entity_id = %s\n", buf);

		jdksavdecc_printer_init(&p, buf, sizeof(buf));
		jdksavdecc_printer_print_eui64(&p, acmp_cmd_resp.listener_entity_id);
		DEBUG_ACMP_PRINTF2("[ACMP] listener_entity_id = %s\n", buf);

		jdksavdecc_printer_init(&p, buf, sizeof(buf));
		jdksavdecc_printer_print_eui64(&p, ctx->entity_id);
		DEBUG_ACMP_PRINTF2("[ACMP] my_entity_id : %s\n", buf);
		}
#endif

		DEBUG_ACMP_PRINTF2("[ACMP] message_type: %d:\n", acmp_cmd_resp.message_type);

		switch (acmp_cmd_resp.message_type) {
		case JDKSAVDECC_ACMP_MESSAGE_TYPE_CONNECT_TX_COMMAND:
		case JDKSAVDECC_ACMP_MESSAGE_TYPE_DISCONNECT_TX_COMMAND:
		case JDKSAVDECC_ACMP_MESSAGE_TYPE_GET_TX_STATE_COMMAND:
		case JDKSAVDECC_ACMP_MESSAGE_TYPE_GET_TX_CONNECTION_COMMAND:
			if (ctx->role & ROLE_TALKER) {
				/* ACMP Talker state machine */
				if (!jdksavdecc_eui64_compare(&acmp_cmd_resp.talker_entity_id, &ctx->entity_id)) {
					acmp_process_talker_state_machine(ctx, &acmp_cmd_resp);
				}
				break;
			}
		case JDKSAVDECC_ACMP_MESSAGE_TYPE_CONNECT_RX_COMMAND:
		case JDKSAVDECC_ACMP_MESSAGE_TYPE_CONNECT_TX_RESPONSE:
		case JDKSAVDECC_ACMP_MESSAGE_TYPE_GET_RX_STATE_COMMAND:
		case JDKSAVDECC_ACMP_MESSAGE_TYPE_DISCONNECT_RX_COMMAND:
		case JDKSAVDECC_ACMP_MESSAGE_TYPE_DISCONNECT_TX_RESPONSE:
			if (ctx->role & ROLE_LISTENER) {
				/* ACMP Listener state machine */
				if (!jdksavdecc_eui64_compare(&acmp_cmd_resp.listener_entity_id, &ctx->entity_id)) {
					acmp_process_listener_state_machine(ctx, &acmp_cmd_resp);
				}
				break;
			}
		default:
			break;
		}
	}

	return 0;
}

int acmp_state_process(struct avdecc_ctx *ctx)
{
	int i;
	struct timespec t;
	jdksavdecc_timestamp_in_microseconds current_time;
	struct acmp_inflight_command *inflight;

	if (!ctx) {
		DEBUG_ACMP_PRINTF1("[ACMP] ctx is NULL in acmp_state_process().\n");
		return -1;
	}

	clock_gettime(CLOCK_MONOTONIC, &t);
	current_time = (t.tv_sec * 1000000) + (t.tv_nsec / 1000);

	for (i = 0; i < MAX_INFLIGHT_CMD_NUM; i++) {
		inflight = &(ctx->dev_e->acmp_listener_inflight_commands[i]);
		if (!inflight->timeout) {
			continue;
		} else {
			if (current_time >= inflight->timeout) {
				switch (inflight->command.message_type) {
				case JDKSAVDECC_ACMP_MESSAGE_TYPE_CONNECT_TX_COMMAND:
					DEBUG_ACMP_PRINTF2("[ACMP] JDKSAVDECC_ACMP_MESSAGE_TYPE_CONNECT_TX_COMMAND:timeout\n");
					acmp_process_connect_tx_timeout(ctx, inflight);
					break;
				case JDKSAVDECC_ACMP_MESSAGE_TYPE_DISCONNECT_TX_COMMAND:
					DEBUG_ACMP_PRINTF2("[ACMP] JDKSAVDECC_ACMP_MESSAGE_TYPE_DISCONNECT_TX_COMMAND:timeout\n");
					acmp_process_disconnect_tx_timeout(ctx, inflight);
					break;
				default:
					break;
				}
			}
		}
	}

	return 0;
}
