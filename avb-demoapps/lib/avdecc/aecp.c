/*
 * Copyright (c) 2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#include <arpa/inet.h>
#include <linux/if_ether.h>

#include "avdecc_internal.h"

#define PAYLOAD_DESCRIPTOR_OFFSET (JDKSAVDECC_AEM_COMMAND_READ_DESCRIPTOR_RESPONSE_OFFSET_DESCRIPTOR)

static int aecp_send_frame(
	struct network_info *net_info,
	const uint8_t *dest_mac,
	struct jdksavdecc_frame *frame,
	int size)
{
	int ret = 0;

	memcpy(frame->dest_address.value, dest_mac, ETH_ALEN);
	memcpy(frame->src_address.value, net_info->mac_addr, ETH_ALEN);

	frame->ethertype = JDKSAVDECC_AVTP_ETHERTYPE;
	frame->length = size;

	ret = send_data(net_info,
			frame->dest_address.value,
			frame->payload,
			frame->length);

	return ret;
}

static inline void aecpdu_fill(
	void *buf,
	uint8_t message_type,
	uint8_t status,
	uint16_t control_data_length,
	struct jdksavdecc_eui64 *target_entity_id,
	struct jdksavdecc_eui64 *controller_entity_id,
	uint16_t sequence_id)
{
	struct jdksavdecc_aecpdu_common *aecpdu_header = buf;
	struct jdksavdecc_aecpdu_common_control_header *header;

	header = &aecpdu_header->header;

	header->cd                  = 1;
	header->subtype             = JDKSAVDECC_SUBTYPE_AECP;
	header->sv                  = 0;
	header->version             = 0;
	header->message_type        = message_type;
	header->status              = status;
	header->control_data_length = control_data_length;
	jdksavdecc_eui64_copy(&header->target_entity_id, target_entity_id);

	aecpdu_header->sequence_id = sequence_id;
	jdksavdecc_eui64_copy(&aecpdu_header->controller_entity_id, controller_entity_id);
}

static inline void aecpdu_aem_fill(
	void *buf,
	uint8_t message_type,
	uint8_t status,
	uint16_t control_data_length,
	struct jdksavdecc_eui64 *target_entity_id,
	struct jdksavdecc_eui64 *controller_entity_id,
	uint16_t sequence_id,
	uint16_t command_type)
{
	struct jdksavdecc_aecpdu_aem *aem = buf;

	aecpdu_fill(
		&aem->aecpdu_header,
		message_type,
		status,
		control_data_length,
		target_entity_id,
		controller_entity_id,
		sequence_id);

	aem->command_type = command_type;
}

static int aecp_send_command_not_implemented(
	struct avdecc_ctx *ctx,
	const struct jdksavdecc_frame *frame,
	struct jdksavdecc_eui64 *controller_entity_id,
	uint16_t sequence_id,
	uint8_t message_type)
{
	int ret = 0;
	struct jdksavdecc_frame resp_frame;
	struct jdksavdecc_aecpdu_common aecpdu_common;

	aecpdu_fill(
		&aecpdu_common,
		message_type,
		JDKSAVDECC_AEM_STATUS_NOT_IMPLEMENTED,
		0,
		&ctx->entity_id,
		controller_entity_id,
		sequence_id);

	resp_frame.length = jdksavdecc_aecpdu_common_write(
		&aecpdu_common,
		resp_frame.payload,
		0,
		sizeof(resp_frame.payload));

	ret = aecp_send_frame(
		&ctx->net_info,
		frame->src_address.value,
		&resp_frame,
		resp_frame.length);

	return ret;
}

static int aecp_send_aem_command_not_implemented(
	struct avdecc_ctx *ctx,
	const struct jdksavdecc_frame *frame,
	struct jdksavdecc_eui64 *controller_entity_id,
	uint16_t sequence_id,
	uint16_t command_type)
{
	int ret = 0;
	struct jdksavdecc_frame resp_frame;
	struct jdksavdecc_aecpdu_aem aemdu;

	aecpdu_aem_fill(
		&aemdu,
		JDKSAVDECC_AECP_MESSAGE_TYPE_AEM_RESPONSE,
		JDKSAVDECC_AEM_STATUS_NOT_IMPLEMENTED,
		0,
		&ctx->entity_id,
		controller_entity_id,
		sequence_id,
		command_type);

	resp_frame.length = jdksavdecc_aecpdu_aem_write(
		&aemdu,
		resp_frame.payload,
		0,
		sizeof(resp_frame.payload));

	ret = aecp_send_frame(
		&ctx->net_info,
		frame->src_address.value,
		&resp_frame,
		resp_frame.length);

	return ret;
}

static struct descriptor_data *aecp_valid_descriptor(
	struct device_entity *dev_e,
	uint16_t rcved_configuration_index,
	uint16_t rcved_desc_type,
	uint16_t rcved_desc_index)
{
	struct configuration_list *config_list = NULL;
	struct descriptor_data *desc_data = NULL;

	if (rcved_desc_type == JDKSAVDECC_DESCRIPTOR_ENTITY) {
		if (dev_e->entity.desc.entity_desc.entity.descriptor_index == rcved_desc_index)
			return &dev_e->entity;
	} else {
		if (rcved_desc_type == JDKSAVDECC_DESCRIPTOR_CONFIGURATION) {
			for (config_list = dev_e->configuration_list; config_list != NULL; config_list = config_list->next) {
				if (config_list->configuration.desc.configuration_desc.configuration.descriptor_index == rcved_desc_index) {
					return &config_list->configuration;
				}
			}
		} else {
			for (config_list = dev_e->configuration_list; config_list != NULL; config_list = config_list->next) {
				if (config_list->configuration.desc.configuration_desc.configuration.descriptor_index == rcved_configuration_index) {
					for (desc_data = config_list->descriptor; desc_data != NULL; desc_data = desc_data->next) {
						if (desc_data->desc.entity_desc.entity.descriptor_type == rcved_desc_type &&
						    desc_data->desc.entity_desc.entity.descriptor_index == rcved_desc_index)
							return desc_data;
					}
				}
			}
		}
	}

	return NULL;
}

/* IEEE1722.1-2013 Chapter.7.4.3 */
static int aecp_process_entity_available(
		struct avdecc_ctx *ctx,
		const struct jdksavdecc_frame *frame,
		struct jdksavdecc_eui64 controller_entity_id,
		uint16_t sequence_id)
{
	int ret = 0;
	struct jdksavdecc_aem_command_entity_available_response aem_entity_available;
	struct jdksavdecc_frame resp_frame;
	struct jdksavdecc_aecpdu_aem *aem_header;

	aem_header = &aem_entity_available.aem_header;

	aecpdu_aem_fill(
		aem_header,
		JDKSAVDECC_AECP_MESSAGE_TYPE_AEM_RESPONSE,
		JDKSAVDECC_AEM_STATUS_SUCCESS,
		JDKSAVDECC_AEM_COMMAND_ENTITY_AVAILABLE_RESPONSE_LEN - JDKSAVDECC_COMMON_CONTROL_HEADER_LEN,
		&ctx->entity_id,
		&controller_entity_id,
		sequence_id,
		JDKSAVDECC_AEM_COMMAND_ENTITY_AVAILABLE);

	resp_frame.length = jdksavdecc_aem_command_entity_available_response_write(
		&aem_entity_available,
		resp_frame.payload,
		0,
		sizeof(resp_frame.payload));

	ret = aecp_send_frame(
		&ctx->net_info,
		frame->src_address.value,
		&resp_frame,
		resp_frame.length);
	if (ret > 0)
		DEBUG_AECP_PRINTF2("[AECP] Sent: response(entity_available)\n");

	return 0;
}

/* IEEE1722.1-2013 Chapter.7.4.5 */
static int aecp_descriptor_data_write(
		uint16_t descriptor_type,
		struct descriptor_data *desc_data,
		void *payload,
		size_t payload_len)
{
	int pos = JDKSAVDECC_AEM_COMMAND_READ_DESCRIPTOR_RESPONSE_OFFSET_DESCRIPTOR;
	int i = 0;

	switch (descriptor_type) {
	case JDKSAVDECC_DESCRIPTOR_ENTITY:
		pos = jdksavdecc_descriptor_entity_write(
			&desc_data->desc.entity_desc.entity,
			payload,
			pos,
			payload_len);
		break;

	case JDKSAVDECC_DESCRIPTOR_CONFIGURATION:
		pos = jdksavdecc_descriptor_configuration_write(
			&desc_data->desc.configuration_desc.configuration,
			payload,
			pos,
			payload_len);
		break;

	case JDKSAVDECC_DESCRIPTOR_AUDIO_UNIT:
		pos = jdksavdecc_descriptor_audio_unit_write(
			&desc_data->desc.audio_unit_desc.audio_unit,
			payload,
			pos,
			payload_len);
		break;

	case JDKSAVDECC_DESCRIPTOR_VIDEO_UNIT:
		pos = jdksavdecc_descriptor_video_write(
			&desc_data->desc.video_unit_desc.video_unit,
			payload,
			pos,
			payload_len);
		break;

	case JDKSAVDECC_DESCRIPTOR_STREAM_INPUT:
	case JDKSAVDECC_DESCRIPTOR_STREAM_OUTPUT:
		pos = jdksavdecc_descriptor_stream_write(
			&desc_data->desc.stream_desc.stream,
			payload,
			pos,
			payload_len);
		break;

	case JDKSAVDECC_DESCRIPTOR_JACK_INPUT:
	case JDKSAVDECC_DESCRIPTOR_JACK_OUTPUT:
		pos = jdksavdecc_descriptor_jack_write(
			&desc_data->desc.jack_desc.jack,
			payload,
			pos,
			payload_len);
		break;

	case JDKSAVDECC_DESCRIPTOR_AVB_INTERFACE:
		pos = jdksavdecc_descriptor_avb_interface_write(
			&desc_data->desc.avb_interface_desc.avb_interface,
			payload,
			pos,
			payload_len);
		break;

	case JDKSAVDECC_DESCRIPTOR_CLOCK_SOURCE:
		pos = jdksavdecc_descriptor_clock_source_write(
			&desc_data->desc.clock_source_desc.clock_source,
			payload,
			pos,
			payload_len);
		break;

	case JDKSAVDECC_DESCRIPTOR_LOCALE:
		pos = jdksavdecc_descriptor_locale_write(
			&desc_data->desc.locale_desc.locale,
			payload,
			pos,
			payload_len);
		break;

	case JDKSAVDECC_DESCRIPTOR_STRINGS:
		pos = jdksavdecc_descriptor_strings_write(
			&desc_data->desc.strings_desc.strings,
			payload,
			pos,
			payload_len);
		break;

	case JDKSAVDECC_DESCRIPTOR_STREAM_PORT_INPUT:
	case JDKSAVDECC_DESCRIPTOR_STREAM_PORT_OUTPUT:
		pos = jdksavdecc_descriptor_stream_port_write(
			&desc_data->desc.stream_port_desc.stream_port,
			payload,
			pos,
			payload_len);
		break;

	case JDKSAVDECC_DESCRIPTOR_AUDIO_CLUSTER:
		pos = jdksavdecc_descriptor_audio_cluster_write(
			&desc_data->desc.audio_cluster_desc.audio_cluster,
			payload,
			pos,
			payload_len);
		break;

	case JDKSAVDECC_DESCRIPTOR_VIDEO_CLUSTER:
		pos = jdksavdecc_descriptor_video_cluster_write(
			&desc_data->desc.video_unit_cluster_desc.video_unit_cluster,
			payload,
			pos,
			payload_len);
		break;

	case JDKSAVDECC_DESCRIPTOR_AUDIO_MAP:
		pos = jdksavdecc_descriptor_audio_map_write(
			&desc_data->desc.audio_map_desc.audio_map,
			payload,
			pos,
			payload_len);
		break;

	case JDKSAVDECC_DESCRIPTOR_VIDEO_MAP:
		pos = jdksavdecc_descriptor_video_map_write(
			&desc_data->desc.video_unit_map_desc.video_unit_map,
			payload,
			pos,
			payload_len);
		break;

	case JDKSAVDECC_DESCRIPTOR_CLOCK_DOMAIN:
		pos = jdksavdecc_descriptor_clock_domain_write(
			&desc_data->desc.clock_domain_desc.clock_domain,
			payload,
			pos,
			payload_len);
		break;

	default:
		DEBUG_AECP_PRINTF1("[AECP] unknown descriptor_type(%d) is set in received AEM_COMMAND\n",
			descriptor_type);
		break;
	}

	switch (descriptor_type) {
	case JDKSAVDECC_DESCRIPTOR_CONFIGURATION:
		for (i = 0; i < desc_data->desc.configuration_desc.configuration.descriptor_counts_count; i++) {
			pos = jdksavdecc_aem_write_descriptor_counts(
					desc_data->desc.configuration_desc.descriptor_counts[i].descriptor_type,
					desc_data->desc.configuration_desc.descriptor_counts[i].count,
					payload,
					pos,
					payload_len);
		}
		break;

	case JDKSAVDECC_DESCRIPTOR_AUDIO_UNIT:
		for (i = 0; i < desc_data->desc.audio_unit_desc.audio_unit.sampling_rates_count; i++) {
			pos = jdksavdecc_uint32_write(
					jdksavdecc_endian_reverse_uint32(&desc_data->desc.audio_unit_desc.sampling_rates[i].sample_rate),
					payload,
					pos,
					payload_len);
		}
		break;

	case JDKSAVDECC_DESCRIPTOR_STREAM_INPUT:
	case JDKSAVDECC_DESCRIPTOR_STREAM_OUTPUT:
		for (i = 0; i < desc_data->desc.stream_desc.stream.number_of_formats; i++) {
			pos = jdksavdecc_uint64_write(
					jdksavdecc_endian_reverse_uint64((uint64_t *)&desc_data->desc.stream_desc.formats[i]),
					payload,
					pos,
					payload_len);
		}
		break;

	case JDKSAVDECC_DESCRIPTOR_VIDEO_CLUSTER:
		for (i = 0; i < desc_data->desc.video_unit_cluster_desc.video_unit_cluster.supported_format_specifics_count; i++)
			pos = jdksavdecc_uint32_write(
					jdksavdecc_endian_reverse_uint32(&desc_data->desc.video_unit_cluster_desc.supported_format_specifics[i]),
					payload,
					pos,
					payload_len);

		for (i = 0; i < desc_data->desc.video_unit_cluster_desc.video_unit_cluster.supported_sampling_rates_count; i++)
			pos = jdksavdecc_uint32_write(
					jdksavdecc_endian_reverse_uint32(&desc_data->desc.video_unit_cluster_desc.supported_sampling_rates[i]),
					payload,
					pos,
					payload_len);

		for (i = 0; i < desc_data->desc.video_unit_cluster_desc.video_unit_cluster.supported_aspect_ratios_count; i++)
			pos = jdksavdecc_uint16_write(
					jdksavdecc_endian_reverse_uint16(&desc_data->desc.video_unit_cluster_desc.supported_aspect_ratios[i]),
					payload,
					pos,
					payload_len);

		for (i = 0; i < desc_data->desc.video_unit_cluster_desc.video_unit_cluster.supported_sizes_count; i++)
			pos = jdksavdecc_uint32_write(
					jdksavdecc_endian_reverse_uint32(&desc_data->desc.video_unit_cluster_desc.supported_sizes[i]),
					payload,
					pos,
					payload_len);

		for (i = 0; i < desc_data->desc.video_unit_cluster_desc.video_unit_cluster.supported_color_spaces_count; i++)
			pos = jdksavdecc_uint16_write(
					jdksavdecc_endian_reverse_uint16(&desc_data->desc.video_unit_cluster_desc.supported_color_spaces[i]),
					payload,
					pos,
					payload_len);
		break;

	case JDKSAVDECC_DESCRIPTOR_AUDIO_MAP:
		for (i = 0; i < desc_data->desc.audio_map_desc.audio_map.number_of_mappings; i++) {
			struct jdksavdecc_audio_mapping tmp_audio_mappings;

			tmp_audio_mappings.mapping_stream_index    = htons(desc_data->desc.audio_map_desc.mappings[i].mapping_stream_index);
			tmp_audio_mappings.mapping_stream_channel  = htons(desc_data->desc.audio_map_desc.mappings[i].mapping_stream_channel);
			tmp_audio_mappings.mapping_cluster_offset  = htons(desc_data->desc.audio_map_desc.mappings[i].mapping_cluster_offset);
			tmp_audio_mappings.mapping_cluster_channel = htons(desc_data->desc.audio_map_desc.mappings[i].mapping_cluster_channel);
			pos = jdksavdecc_audio_mapping_write(&tmp_audio_mappings,
					payload,
					pos,
					payload_len);
		}
		break;

	case JDKSAVDECC_DESCRIPTOR_VIDEO_MAP:
		for (i = 0; i < desc_data->desc.video_unit_map_desc.video_unit_map.number_of_mappings; i++) {
			struct jdksavdecc_video_mapping tmp_video_mappings;

			tmp_video_mappings.mapping_stream_index      = htons(desc_data->desc.video_unit_map_desc.mappings[i].mapping_stream_index);
			tmp_video_mappings.mapping_program_stream    = htons(desc_data->desc.video_unit_map_desc.mappings[i].mapping_program_stream);
			tmp_video_mappings.mapping_elementary_stream = htons(desc_data->desc.video_unit_map_desc.mappings[i].mapping_elementary_stream);
			tmp_video_mappings.mapping_cluster_offset    = htons(desc_data->desc.video_unit_map_desc.mappings[i].mapping_cluster_offset);
			pos = jdksavdecc_video_mapping_write(&tmp_video_mappings,
					payload,
					pos,
					payload_len);
		}
		break;

	case JDKSAVDECC_DESCRIPTOR_CLOCK_DOMAIN:
		for (i = 0; i < desc_data->desc.clock_domain_desc.clock_domain.clock_sources_count; i++) {
			pos = jdksavdecc_uint16_write(
					jdksavdecc_endian_reverse_uint16(&desc_data->desc.clock_domain_desc.clock_sources[i]),
					payload,
					pos,
					payload_len);
		}
		break;

	default:
		break;
	}

	return pos - JDKSAVDECC_AEM_COMMAND_READ_DESCRIPTOR_RESPONSE_OFFSET_DESCRIPTOR;
}

/* IEEE1722.1-2013 Chapter.7.4.5 */
static int aecp_process_read_descriptor(
		struct avdecc_ctx *ctx,
		const struct jdksavdecc_frame *frame,
		struct jdksavdecc_eui64 controller_entity_id,
		uint16_t sequence_id)
{
	int ret = 0;
	int desc_size = 0;
	int status;

	struct jdksavdecc_aem_command_read_descriptor p;
	struct jdksavdecc_aem_command_read_descriptor_response aem_read_res;
	struct jdksavdecc_aecpdu_common *aecpdu_header;
	struct jdksavdecc_frame resp_frame;
	struct descriptor_data *desc_data;

	aecpdu_header = &aem_read_res.aem_header.aecpdu_header;

	if (jdksavdecc_aem_command_read_descriptor_read(&p, frame->payload, 0, frame->length) < 0)
		return 0;

	aecpdu_aem_fill(
		&aem_read_res,
		JDKSAVDECC_AECP_MESSAGE_TYPE_AEM_RESPONSE,
		JDKSAVDECC_AEM_STATUS_SUCCESS,
		0,
		&ctx->entity_id,
		&controller_entity_id,
		sequence_id,
		JDKSAVDECC_AEM_COMMAND_READ_DESCRIPTOR);

	aem_read_res.reserved = 0;

	desc_data = aecp_valid_descriptor(
		ctx->dev_e,
		p.configuration_index,
		p.descriptor_type,
		p.descriptor_index);
	if (!desc_data) {
		status = JDKSAVDECC_AEM_STATUS_NO_SUCH_DESCRIPTOR;
	} else {
		desc_size = aecp_descriptor_data_write(
			p.descriptor_type,
			desc_data,
			resp_frame.payload,
			sizeof(resp_frame.payload));
		if (desc_size) {
			status = JDKSAVDECC_AEM_STATUS_SUCCESS;
		} else {
			status = JDKSAVDECC_AEM_STATUS_NOT_SUPPORTED;
		}
	}

	if ((p.descriptor_type == JDKSAVDECC_DESCRIPTOR_ENTITY) ||
	    (p.descriptor_type == JDKSAVDECC_DESCRIPTOR_CONFIGURATION))
		aem_read_res.configuration_index = 0;
	else
		aem_read_res.configuration_index = p.configuration_index;

	aecpdu_header->header.status = status;
	aecpdu_header->header.control_data_length =
		desc_size + JDKSAVDECC_AEM_COMMAND_READ_DESCRIPTOR_RESPONSE_LEN - JDKSAVDECC_COMMON_CONTROL_HEADER_LEN;

	ret = jdksavdecc_aem_command_read_descriptor_response_write(
		&aem_read_res,
		resp_frame.payload,
		0,
		sizeof(resp_frame.payload));

	resp_frame.length = ret + desc_size;

	ret = aecp_send_frame(
		&ctx->net_info,
		frame->src_address.value,
		&resp_frame,
		resp_frame.length);
	if (ret > 0)
		DEBUG_AECP_PRINTF2("[AECP] Sent: response(%d)\n", p.descriptor_type);

	return 0;
}

/* IEEE1722.1-2013 Figure 9.8 RECEIVED_COMMAND */
static int aecp_process_aem_command(
		struct avdecc_ctx *ctx,
		const struct jdksavdecc_frame *frame)
{
	struct jdksavdecc_aecpdu_aem aemdu;

	if (jdksavdecc_aecpdu_aem_read(
			&aemdu,
			frame->payload,
			0,
			frame->length) > 0) {
		if (jdksavdecc_eui64_compare(&aemdu.aecpdu_header.header.target_entity_id, &ctx->entity_id)) {
			DEBUG_AECP_PRINTF2("[AECP] target_entity_id is not my entity_id\n");
#if DEBUG_AECP
			{
			struct jdksavdecc_printer p;
			char buf[256];
			jdksavdecc_printer_init(&p, buf, sizeof(buf));
			jdksavdecc_printer_print_eui64(&p, aemdu.aecpdu_header.header.target_entity_id);
			DEBUG_AECP_PRINTF2("[AECP] target_entity_id = %s\n", buf);

			jdksavdecc_printer_init(&p, buf, sizeof(buf));
			jdksavdecc_printer_print_eui64(&p, ctx->entity_id);
			DEBUG_AECP_PRINTF2("[AECP] my_entity_id : %s\n", buf);
			}
#endif
		} else {
			switch (aemdu.command_type) {
			/* The following processing are IEEE1722.1-2013 Chapter.9.2.2.3.1.3.2 */
			case JDKSAVDECC_AEM_COMMAND_ENTITY_AVAILABLE:
				DEBUG_AECP_PRINTF2("[AECP] aecp_process_entity_available:\n");
				aecp_process_entity_available(
						ctx,
						frame,
						aemdu.aecpdu_header.controller_entity_id,
						aemdu.aecpdu_header.sequence_id);
				break;
			case JDKSAVDECC_AEM_COMMAND_READ_DESCRIPTOR:
				DEBUG_AECP_PRINTF2("[AECP] aecp_process_read_descriptor:\n");
				aecp_process_read_descriptor(
						ctx,
						frame,
						aemdu.aecpdu_header.controller_entity_id,
						aemdu.aecpdu_header.sequence_id);
				break;

			default:
				DEBUG_AECP_PRINTF1("[AECP] received not implemented aemdu command_type:%d\n",
						aemdu.command_type);
				aecp_send_aem_command_not_implemented(
						ctx,
						frame,
						&aemdu.aecpdu_header.controller_entity_id,
						aemdu.aecpdu_header.sequence_id,
						aemdu.command_type);
				break;
			}
		}
	}

	return 0;
}

int aecp_receive_process(
		struct avdecc_ctx *ctx,
		const struct jdksavdecc_frame *frame)
{
	struct jdksavdecc_aecpdu_common aecpdu_common;

	if (!ctx) {
		DEBUG_ACMP_PRINTF1("[AECP] ctx is NULL in aecp_receive_process().\n");
		return -1;
	}

	if (!frame) {
		DEBUG_ACMP_PRINTF1("[AECP] frame is NULL in aecp_receive_process().\n");
		return -1;
	}

	if (jdksavdecc_aecpdu_common_read(
			&aecpdu_common,
			frame->payload,
			0,
			frame->length) < 0)
		return 0; /* Invalid */

	if (jdksavdecc_eui64_compare(&aecpdu_common.header.target_entity_id, &ctx->entity_id))
		return 0; /* This command was not to target my Entity */

	switch (aecpdu_common.header.message_type) {
	case JDKSAVDECC_AECP_MESSAGE_TYPE_AEM_COMMAND:
		aecp_process_aem_command(ctx, frame);
		break;

	case JDKSAVDECC_AECP_MESSAGE_TYPE_AEM_RESPONSE:
		/*
		 * This library does not support Controller role.
		 * If support Contoller role, this messages
		 * is to need implement.
		 */
		break;

	case JDKSAVDECC_AECP_MESSAGE_TYPE_ADDRESS_ACCESS_COMMAND:
	case JDKSAVDECC_AECP_MESSAGE_TYPE_AVC_COMMAND:
	case JDKSAVDECC_AECP_MESSAGE_TYPE_VENDOR_UNIQUE_COMMAND:
	case JDKSAVDECC_AECP_MESSAGE_TYPE_HDCP_APM_COMMAND:
	case JDKSAVDECC_AECP_MESSAGE_TYPE_EXTENDED_COMMAND:
		aecp_send_command_not_implemented(
			ctx,
			frame,
			&aecpdu_common.controller_entity_id,
			aecpdu_common.sequence_id,
			aecpdu_common.header.message_type + 1);
		break;

	case JDKSAVDECC_AECP_MESSAGE_TYPE_ADDRESS_ACCESS_RESPONSE:
	case JDKSAVDECC_AECP_MESSAGE_TYPE_AVC_RESPONSE:
	case JDKSAVDECC_AECP_MESSAGE_TYPE_VENDOR_UNIQUE_RESPONSE:
	case JDKSAVDECC_AECP_MESSAGE_TYPE_HDCP_APM_RESPONSE:
	case JDKSAVDECC_AECP_MESSAGE_TYPE_EXTENDED_RESPONSE:
		/* these messages does not support */
		break;
	default:
		break;
	}

	return 0;
}

int aecp_state_process(struct avdecc_ctx *ctx)
{
	/*
	 * This library does not support Controller role and
	 * AEM COMMAND ACQUIRE/LOCK Entity. If support these
	 * feature, this function is to need implement.
	 */
	return 0;
}
