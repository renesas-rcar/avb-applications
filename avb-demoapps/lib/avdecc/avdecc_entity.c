/*
 * Copyright (c) 2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */
#include <net/if.h>
#include <linux/if_ether.h>
#include <yaml.h>

#include "avdecc_internal.h"

static int assigned_string_data(const char *value, void *d);
static int assigned_valiable_data(const char *value, void *d);
static int assigned_eui48_data(const char *value, void *d);
static int assigned_eui64_data(const char *value, void *d);
static int assigned_uint32_data(const char *value, void *d);

#define AVDECC_DESC_PENTRY(type, member, desc) { # member, offsetof(desc, member), assigned_##type##_data }
#define AVDECC_DESC_ENTITY_PENTRY(type, member) AVDECC_DESC_PENTRY(type, member, struct jdksavdecc_descriptor_entity)
#define AVDECC_DESC_CONFIGURATION_PENTRY(type, member) AVDECC_DESC_PENTRY(type, member, struct jdksavdecc_descriptor_configuration)
#define AVDECC_DESC_AUDIO_UNIT_PENTRY(type, member) AVDECC_DESC_PENTRY(type, member, struct jdksavdecc_descriptor_audio_unit)
#define AVDECC_DESC_VIDEO_UNIT_PENTRY(type, member) AVDECC_DESC_PENTRY(type, member, struct jdksavdecc_descriptor_video_unit)
#define AVDECC_DESC_STREAM_PENTRY(type, member) AVDECC_DESC_PENTRY(type, member, struct jdksavdecc_descriptor_stream)
#define AVDECC_DESC_JACK_PENTRY(type, member) AVDECC_DESC_PENTRY(type, member, struct jdksavdecc_descriptor_jack)
#define AVDECC_DESC_AVB_INTERFACE_PENTRY(type, member) AVDECC_DESC_PENTRY(type, member, struct jdksavdecc_descriptor_avb_interface)
#define AVDECC_DESC_CLOCK_SOURCE_PENTRY(type, member) AVDECC_DESC_PENTRY(type, member, struct jdksavdecc_descriptor_clock_source)
#define AVDECC_DESC_LOCALE_PENTRY(type, member) AVDECC_DESC_PENTRY(type, member, struct jdksavdecc_descriptor_locale)
#define AVDECC_DESC_STRINGS_PENTRY(type, member) AVDECC_DESC_PENTRY(type, member, struct jdksavdecc_descriptor_strings)
#define AVDECC_DESC_STREAM_PORT_PENTRY(type, member) AVDECC_DESC_PENTRY(type, member, struct jdksavdecc_descriptor_stream_port)
#define AVDECC_DESC_EXTERNAL_PORT_PENTRY(type, member) AVDECC_DESC_PENTRY(type, member, struct jdksavdecc_descriptor_external_port)
#define AVDECC_DESC_INTERNAL_PORT_PENTRY(type, member) AVDECC_DESC_PENTRY(type, member, struct jdksavdecc_descriptor_internal_port)
#define AVDECC_DESC_AUDIO_CLUSTER_PENTRY(type, member) AVDECC_DESC_PENTRY(type, member, struct jdksavdecc_descriptor_audio_cluster)
#define AVDECC_DESC_VIDEO_CLUSTER_PENTRY(type, member) AVDECC_DESC_PENTRY(type, member, struct jdksavdecc_descriptor_video_unit_cluster)
#define AVDECC_DESC_AUDIO_MAP_PENTRY(type, member) AVDECC_DESC_PENTRY(type, member, struct jdksavdecc_descriptor_audio_map)
#define AVDECC_DESC_VIDEO_MAP_PENTRY(type, member) AVDECC_DESC_PENTRY(type, member, struct jdksavdecc_descriptor_video_unit_map)
#define AVDECC_DESC_CLOCK_DOMAIN_PENTRY(type, member) AVDECC_DESC_PENTRY(type, member, struct jdksavdecc_descriptor_clock_domain)

struct avdecc_desc_parse_entry {
	const char *name;
	size_t offset;
	int (*parse)(const char *value, void *data);
};

static struct avdecc_desc_parse_entry avdecc_desc_parse_table_entity[] = {
	AVDECC_DESC_ENTITY_PENTRY(uint32, descriptor_type),
	AVDECC_DESC_ENTITY_PENTRY(uint32, descriptor_index),
	AVDECC_DESC_ENTITY_PENTRY(eui64, entity_id),
	AVDECC_DESC_ENTITY_PENTRY(eui64, entity_model_id),
	AVDECC_DESC_ENTITY_PENTRY(uint32, entity_capabilities),
	AVDECC_DESC_ENTITY_PENTRY(uint32, talker_stream_sources),
	AVDECC_DESC_ENTITY_PENTRY(uint32, talker_capabilities),
	AVDECC_DESC_ENTITY_PENTRY(uint32, listener_stream_sinks),
	AVDECC_DESC_ENTITY_PENTRY(uint32, listener_capabilities),
	AVDECC_DESC_ENTITY_PENTRY(uint32, controller_capabilities),
	AVDECC_DESC_ENTITY_PENTRY(uint32, available_index),
	AVDECC_DESC_ENTITY_PENTRY(eui64, association_id),
	AVDECC_DESC_ENTITY_PENTRY(string, entity_name),
	AVDECC_DESC_ENTITY_PENTRY(uint32, vendor_name_string),
	AVDECC_DESC_ENTITY_PENTRY(uint32, model_name_string),
	AVDECC_DESC_ENTITY_PENTRY(string, firmware_version),
	AVDECC_DESC_ENTITY_PENTRY(string, group_name),
	AVDECC_DESC_ENTITY_PENTRY(string, serial_number),
	AVDECC_DESC_ENTITY_PENTRY(uint32, configurations_count),
	AVDECC_DESC_ENTITY_PENTRY(uint32, current_configuration),
};

static struct avdecc_desc_parse_entry avdecc_desc_parse_table_configuration[] = {
	AVDECC_DESC_CONFIGURATION_PENTRY(uint32, descriptor_type),
	AVDECC_DESC_CONFIGURATION_PENTRY(uint32, descriptor_index),
	AVDECC_DESC_CONFIGURATION_PENTRY(string, object_name),
	AVDECC_DESC_CONFIGURATION_PENTRY(uint32, localized_description),
	AVDECC_DESC_CONFIGURATION_PENTRY(uint32, descriptor_counts_count),
	AVDECC_DESC_CONFIGURATION_PENTRY(uint32, descriptor_counts_offset),
};

static struct avdecc_desc_parse_entry avdecc_desc_parse_table_audio_unit[] = {
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, descriptor_type),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, descriptor_index),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(string, object_name),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, localized_description),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, clock_domain_index),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, number_of_stream_input_ports),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, base_stream_input_port),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, number_of_stream_output_ports),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, base_stream_output_port),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, number_of_external_input_ports),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, base_external_input_port),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, number_of_external_output_ports),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, base_external_output_port),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, number_of_internal_input_ports),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, base_internal_input_port),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, number_of_internal_output_ports),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, base_internal_output_port),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, number_of_controls),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, base_control),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, number_of_signal_selectors),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, base_signal_selector),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, number_of_mixers),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, base_mixer),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, number_of_matrices),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, base_matrix),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, number_of_splitters),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, base_splitter),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, number_of_combiners),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, base_combiner),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, number_of_demultiplexers),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, base_demultiplexer),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, number_of_multiplexers),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, base_multiplexer),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, number_of_transcoders),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, base_transcoder),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, number_of_control_blocks),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, base_control_block),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, current_sampling_rate),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, sampling_rates_offset),
	AVDECC_DESC_AUDIO_UNIT_PENTRY(uint32, sampling_rates_count),
};

static struct avdecc_desc_parse_entry avdecc_desc_parse_table_video_unit[] = {
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, descriptor_type),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, descriptor_index),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(string, object_name),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, localized_description),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, clock_domain_index),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, number_of_stream_input_ports),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, base_stream_input_port),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, number_of_stream_output_ports),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, base_stream_output_port),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, number_of_external_input_ports),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, base_external_input_port),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, number_of_external_output_ports),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, base_external_output_port),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, number_of_internal_input_ports),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, base_internal_input_port),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, number_of_internal_output_ports),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, base_internal_output_port),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, number_of_controls),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, base_control),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, number_of_signal_selectors),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, base_signal_selector),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, number_of_mixers),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, base_mixer),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, number_of_matrices),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, base_matrix),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, number_of_splitters),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, base_splitter),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, number_of_combiners),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, base_combiner),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, number_of_demultiplexers),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, base_demultiplexer),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, number_of_multiplexers),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, base_multiplexer),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, number_of_transcoders),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, base_transcoder),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, number_of_control_blocks),
	AVDECC_DESC_VIDEO_UNIT_PENTRY(uint32, base_control_block),
};

static struct avdecc_desc_parse_entry avdecc_desc_parse_table_stream[] = {
	AVDECC_DESC_STREAM_PENTRY(uint32, descriptor_type),
	AVDECC_DESC_STREAM_PENTRY(uint32, descriptor_index),
	AVDECC_DESC_STREAM_PENTRY(string, object_name),
	AVDECC_DESC_STREAM_PENTRY(uint32, localized_description),
	AVDECC_DESC_STREAM_PENTRY(uint32, clock_domain_index),
	AVDECC_DESC_STREAM_PENTRY(uint32, stream_flags),
	AVDECC_DESC_STREAM_PENTRY(eui64, current_format),
	AVDECC_DESC_STREAM_PENTRY(uint32, formats_offset),
	AVDECC_DESC_STREAM_PENTRY(uint32, number_of_formats),
	AVDECC_DESC_STREAM_PENTRY(eui64, backup_talker_entity_id_0),
	AVDECC_DESC_STREAM_PENTRY(uint32, backup_talker_unique_id_0),
	AVDECC_DESC_STREAM_PENTRY(eui64, backup_talker_entity_id_1),
	AVDECC_DESC_STREAM_PENTRY(uint32, backup_talker_unique_id_1),
	AVDECC_DESC_STREAM_PENTRY(eui64, backup_talker_entity_id_2),
	AVDECC_DESC_STREAM_PENTRY(uint32, backup_talker_unique_id_2),
	AVDECC_DESC_STREAM_PENTRY(eui64, backedup_talker_entity_id),
	AVDECC_DESC_STREAM_PENTRY(uint32, backedup_talker_unique),
	AVDECC_DESC_STREAM_PENTRY(uint32, avb_interface_index),
	AVDECC_DESC_STREAM_PENTRY(uint32, buffer_length),
};

static struct avdecc_desc_parse_entry avdecc_desc_parse_table_jack[] = {
	AVDECC_DESC_JACK_PENTRY(uint32, descriptor_type),
	AVDECC_DESC_JACK_PENTRY(uint32, descriptor_index),
	AVDECC_DESC_JACK_PENTRY(string, object_name),
	AVDECC_DESC_JACK_PENTRY(uint32, localized_description),
	AVDECC_DESC_JACK_PENTRY(uint32, jack_flags),
	AVDECC_DESC_JACK_PENTRY(uint32, jack_type),
	AVDECC_DESC_JACK_PENTRY(uint32, number_of_controls),
	AVDECC_DESC_JACK_PENTRY(uint32, base_control),
};

static struct avdecc_desc_parse_entry avdecc_desc_parse_table_avb_interface[] = {
	AVDECC_DESC_AVB_INTERFACE_PENTRY(uint32, descriptor_type),
	AVDECC_DESC_AVB_INTERFACE_PENTRY(uint32, descriptor_index),
	AVDECC_DESC_AVB_INTERFACE_PENTRY(string, object_name),
	AVDECC_DESC_AVB_INTERFACE_PENTRY(uint32, localized_description),
	AVDECC_DESC_AVB_INTERFACE_PENTRY(eui48, mac_address),
	AVDECC_DESC_AVB_INTERFACE_PENTRY(uint32, interface_flags),
	AVDECC_DESC_AVB_INTERFACE_PENTRY(eui64, clock_identity),
	AVDECC_DESC_AVB_INTERFACE_PENTRY(uint32, priority1),
	AVDECC_DESC_AVB_INTERFACE_PENTRY(uint32, clock_class),
	AVDECC_DESC_AVB_INTERFACE_PENTRY(uint32, offset_scaled_log_variance),
	AVDECC_DESC_AVB_INTERFACE_PENTRY(uint32, clock_accuracy),
	AVDECC_DESC_AVB_INTERFACE_PENTRY(uint32, priority2),
	AVDECC_DESC_AVB_INTERFACE_PENTRY(uint32, domain_number),
	AVDECC_DESC_AVB_INTERFACE_PENTRY(uint32, log_sync_interval),
	AVDECC_DESC_AVB_INTERFACE_PENTRY(uint32, log_announce_interval),
	AVDECC_DESC_AVB_INTERFACE_PENTRY(uint32, log_pdelay_interval),
	AVDECC_DESC_AVB_INTERFACE_PENTRY(uint32, port_number),
};

static struct avdecc_desc_parse_entry avdecc_desc_parse_table_clock_source[] = {
	AVDECC_DESC_CLOCK_SOURCE_PENTRY(uint32, descriptor_type),
	AVDECC_DESC_CLOCK_SOURCE_PENTRY(uint32, descriptor_index),
	AVDECC_DESC_CLOCK_SOURCE_PENTRY(string, object_name),
	AVDECC_DESC_CLOCK_SOURCE_PENTRY(uint32, localized_description),
	AVDECC_DESC_CLOCK_SOURCE_PENTRY(uint32, clock_source_flags),
	AVDECC_DESC_CLOCK_SOURCE_PENTRY(eui64, clock_source_identifier),
	AVDECC_DESC_CLOCK_SOURCE_PENTRY(uint32, clock_source_location_type),
	AVDECC_DESC_CLOCK_SOURCE_PENTRY(uint32, clock_source_location_index),
};

static struct avdecc_desc_parse_entry avdecc_desc_parse_table_locale[] = {
	AVDECC_DESC_LOCALE_PENTRY(uint32, descriptor_type),
	AVDECC_DESC_LOCALE_PENTRY(uint32, descriptor_index),
	AVDECC_DESC_LOCALE_PENTRY(string, locale_identifier),
	AVDECC_DESC_LOCALE_PENTRY(uint32, number_of_strings),
	AVDECC_DESC_LOCALE_PENTRY(uint32, base_strings),
};

static struct avdecc_desc_parse_entry avdecc_desc_parse_table_strings[] = {
	AVDECC_DESC_STRINGS_PENTRY(uint32, descriptor_type),
	AVDECC_DESC_STRINGS_PENTRY(uint32, descriptor_index),
	AVDECC_DESC_STRINGS_PENTRY(string, string_0),
	AVDECC_DESC_STRINGS_PENTRY(string, string_1),
	AVDECC_DESC_STRINGS_PENTRY(string, string_2),
	AVDECC_DESC_STRINGS_PENTRY(string, string_3),
	AVDECC_DESC_STRINGS_PENTRY(string, string_4),
	AVDECC_DESC_STRINGS_PENTRY(string, string_5),
	AVDECC_DESC_STRINGS_PENTRY(string, string_6),
};

static struct avdecc_desc_parse_entry avdecc_desc_parse_table_stream_port[] = {
	AVDECC_DESC_STREAM_PORT_PENTRY(uint32, descriptor_type),
	AVDECC_DESC_STREAM_PORT_PENTRY(uint32, descriptor_index),
	AVDECC_DESC_STREAM_PORT_PENTRY(uint32, clock_domain_index),
	AVDECC_DESC_STREAM_PORT_PENTRY(uint32, port_flags),
	AVDECC_DESC_STREAM_PORT_PENTRY(uint32, number_of_controls),
	AVDECC_DESC_STREAM_PORT_PENTRY(uint32, base_control),
	AVDECC_DESC_STREAM_PORT_PENTRY(uint32, number_of_clusters),
	AVDECC_DESC_STREAM_PORT_PENTRY(uint32, base_cluster),
	AVDECC_DESC_STREAM_PORT_PENTRY(uint32, number_of_maps),
	AVDECC_DESC_STREAM_PORT_PENTRY(uint32, base_map),
};

static struct avdecc_desc_parse_entry avdecc_desc_parse_table_audio_cluster[] = {
	AVDECC_DESC_AUDIO_CLUSTER_PENTRY(uint32, descriptor_type),
	AVDECC_DESC_AUDIO_CLUSTER_PENTRY(uint32, descriptor_index),
	AVDECC_DESC_AUDIO_CLUSTER_PENTRY(string, object_name),
	AVDECC_DESC_AUDIO_CLUSTER_PENTRY(uint32, localized_description),
	AVDECC_DESC_AUDIO_CLUSTER_PENTRY(uint32, signal_type),
	AVDECC_DESC_AUDIO_CLUSTER_PENTRY(uint32, signal_index),
	AVDECC_DESC_AUDIO_CLUSTER_PENTRY(uint32, signal_output),
	AVDECC_DESC_AUDIO_CLUSTER_PENTRY(uint32, path_latency),
	AVDECC_DESC_AUDIO_CLUSTER_PENTRY(uint32, block_latency),
	AVDECC_DESC_AUDIO_CLUSTER_PENTRY(uint32, channel_count),
	AVDECC_DESC_AUDIO_CLUSTER_PENTRY(uint32, format),
};

static struct avdecc_desc_parse_entry avdecc_desc_parse_table_video_cluster[] = {
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, descriptor_type),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, descriptor_index),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(string, object_name),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, localized_description),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, signal_type),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, signal_index),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, signal_output),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, path_latency),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, block_latency),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, format),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, current_format_specific),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, supported_format_specifics_offset),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, supported_format_specifics_count),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, current_sampling_rate),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, supported_sampling_rates_offset),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, supported_sampling_rates_count),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, current_aspect_ratio),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, supported_aspect_ratios_offset),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, supported_aspect_ratios_count),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, current_size),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, supported_sizes_offset),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, supported_sizes_count),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, current_color_space),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, supported_color_spaces_offset),
	AVDECC_DESC_VIDEO_CLUSTER_PENTRY(uint32, supported_color_spaces_count),
};

static struct avdecc_desc_parse_entry avdecc_desc_parse_table_audio_map[] = {
	AVDECC_DESC_AUDIO_MAP_PENTRY(uint32, descriptor_type),
	AVDECC_DESC_AUDIO_MAP_PENTRY(uint32, descriptor_index),
	AVDECC_DESC_AUDIO_MAP_PENTRY(uint32, mappings_offset),
	AVDECC_DESC_AUDIO_MAP_PENTRY(uint32, number_of_mappings),
};

static struct avdecc_desc_parse_entry avdecc_desc_parse_table_video_map[] = {
	AVDECC_DESC_VIDEO_MAP_PENTRY(uint32, descriptor_type),
	AVDECC_DESC_VIDEO_MAP_PENTRY(uint32, descriptor_index),
	AVDECC_DESC_VIDEO_MAP_PENTRY(uint32, mappings_offset),
	AVDECC_DESC_VIDEO_MAP_PENTRY(uint32, number_of_mappings),
};

static struct avdecc_desc_parse_entry avdecc_desc_parse_table_clock_domain[] = {
	AVDECC_DESC_CLOCK_DOMAIN_PENTRY(uint32, descriptor_type),
	AVDECC_DESC_CLOCK_DOMAIN_PENTRY(uint32, descriptor_index),
	AVDECC_DESC_CLOCK_DOMAIN_PENTRY(string, object_name),
	AVDECC_DESC_CLOCK_DOMAIN_PENTRY(uint32, localized_description),
	AVDECC_DESC_CLOCK_DOMAIN_PENTRY(uint32, clock_source_index),
	AVDECC_DESC_CLOCK_DOMAIN_PENTRY(uint32, clock_sources_offset),
	AVDECC_DESC_CLOCK_DOMAIN_PENTRY(uint32, clock_sources_count),
};

struct parse_descriptor {
	char *name;
	struct avdecc_desc_parse_entry *entry;
	int entry_size;
};

static struct parse_descriptor d_table[JDKSAVDECC_NUM_DESCRIPTOR_TYPES] = {
	[JDKSAVDECC_DESCRIPTOR_ENTITY]                   = { "entity",                  avdecc_desc_parse_table_entity,                 ARRAY_SIZE(avdecc_desc_parse_table_entity)                },
	[JDKSAVDECC_DESCRIPTOR_CONFIGURATION]            = { "configuration",           avdecc_desc_parse_table_configuration,          ARRAY_SIZE(avdecc_desc_parse_table_configuration)         },
	[JDKSAVDECC_DESCRIPTOR_AUDIO_UNIT]               = { "audio_unit",              avdecc_desc_parse_table_audio_unit,             ARRAY_SIZE(avdecc_desc_parse_table_audio_unit)            },
	[JDKSAVDECC_DESCRIPTOR_VIDEO_UNIT]               = { "video_unit",              avdecc_desc_parse_table_video_unit,             ARRAY_SIZE(avdecc_desc_parse_table_video_unit)            },
	[JDKSAVDECC_DESCRIPTOR_SENSOR_UNIT]              = { "sensor_unit",             NULL,                                           0                                                         },
	[JDKSAVDECC_DESCRIPTOR_STREAM_INPUT]             = { "stream_input",            avdecc_desc_parse_table_stream,                 ARRAY_SIZE(avdecc_desc_parse_table_stream)                },
	[JDKSAVDECC_DESCRIPTOR_STREAM_OUTPUT]            = { "stream_output",           avdecc_desc_parse_table_stream,                 ARRAY_SIZE(avdecc_desc_parse_table_stream)                },
	[JDKSAVDECC_DESCRIPTOR_JACK_INPUT]               = { "jack_input",              avdecc_desc_parse_table_jack,                   ARRAY_SIZE(avdecc_desc_parse_table_jack)                  },
	[JDKSAVDECC_DESCRIPTOR_JACK_OUTPUT]              = { "jack_output",             avdecc_desc_parse_table_jack,                   ARRAY_SIZE(avdecc_desc_parse_table_jack)                  },
	[JDKSAVDECC_DESCRIPTOR_AVB_INTERFACE]            = { "avb_interface",           avdecc_desc_parse_table_avb_interface,          ARRAY_SIZE(avdecc_desc_parse_table_avb_interface)         },
	[JDKSAVDECC_DESCRIPTOR_CLOCK_SOURCE]             = { "clock_source",            avdecc_desc_parse_table_clock_source,           ARRAY_SIZE(avdecc_desc_parse_table_clock_source)          },
	[JDKSAVDECC_DESCRIPTOR_MEMORY_OBJECT]            = { "memory_object",           NULL,                                           0                                                         },
	[JDKSAVDECC_DESCRIPTOR_LOCALE]                   = { "locale",                  avdecc_desc_parse_table_locale,                 ARRAY_SIZE(avdecc_desc_parse_table_locale)                },
	[JDKSAVDECC_DESCRIPTOR_STRINGS]                  = { "strings",                 avdecc_desc_parse_table_strings,                ARRAY_SIZE(avdecc_desc_parse_table_strings)               },
	[JDKSAVDECC_DESCRIPTOR_STREAM_PORT_INPUT]        = { "stream_port_input",       avdecc_desc_parse_table_stream_port,            ARRAY_SIZE(avdecc_desc_parse_table_stream_port)           },
	[JDKSAVDECC_DESCRIPTOR_STREAM_PORT_OUTPUT]       = { "stream_port_output",      avdecc_desc_parse_table_stream_port,            ARRAY_SIZE(avdecc_desc_parse_table_stream_port)           },
	[JDKSAVDECC_DESCRIPTOR_EXTERNAL_PORT_INPUT]      = { "external_port_input",     NULL,                                           0                                                         },
	[JDKSAVDECC_DESCRIPTOR_EXTERNAL_PORT_OUTPUT]     = { "external_port_output",    NULL,                                           0                                                         },
	[JDKSAVDECC_DESCRIPTOR_INTERNAL_PORT_INPUT]      = { "internal_port_input",     NULL,                                           0                                                         },
	[JDKSAVDECC_DESCRIPTOR_INTERNAL_PORT_OUTPUT]     = { "internal_port_output",    NULL,                                           0                                                         },
	[JDKSAVDECC_DESCRIPTOR_AUDIO_CLUSTER]            = { "audio_cluster",           avdecc_desc_parse_table_audio_cluster,          ARRAY_SIZE(avdecc_desc_parse_table_audio_cluster)         },
	[JDKSAVDECC_DESCRIPTOR_VIDEO_CLUSTER]            = { "video_cluster",           avdecc_desc_parse_table_video_cluster,          ARRAY_SIZE(avdecc_desc_parse_table_video_cluster)         },
	[JDKSAVDECC_DESCRIPTOR_SENSOR_CLUSTER]           = { "sensor_cluster",          NULL,                                           0                                                         },
	[JDKSAVDECC_DESCRIPTOR_AUDIO_MAP]                = { "audio_map",               avdecc_desc_parse_table_audio_map,              ARRAY_SIZE(avdecc_desc_parse_table_audio_map)             },
	[JDKSAVDECC_DESCRIPTOR_VIDEO_MAP]                = { "video_map",               avdecc_desc_parse_table_video_map,              ARRAY_SIZE(avdecc_desc_parse_table_video_map)             },
	[JDKSAVDECC_DESCRIPTOR_SENSOR_MAP]               = { "sensor_map",              NULL,                                           0                                                         },
	[JDKSAVDECC_DESCRIPTOR_CONTROL]                  = { "control",                 NULL,                                           0                                                         },
	[JDKSAVDECC_DESCRIPTOR_SIGNAL_SELECTOR]          = { "signal_selector",         NULL,                                           0                                                         },
	[JDKSAVDECC_DESCRIPTOR_MIXER]                    = { "mixer",                   NULL,                                           0                                                         },
	[JDKSAVDECC_DESCRIPTOR_MATRIX]                   = { "matrix",                  NULL,                                           0                                                         },
	[JDKSAVDECC_DESCRIPTOR_MATRIX_SIGNAL]            = { "matrix_signal",           NULL,                                           0                                                         },
	[JDKSAVDECC_DESCRIPTOR_SIGNAL_SPLITTER]          = { "signal_splitter",         NULL,                                           0                                                         },
	[JDKSAVDECC_DESCRIPTOR_SIGNAL_COMBINER]          = { "signal_combiner",         NULL,                                           0                                                         },
	[JDKSAVDECC_DESCRIPTOR_SIGNAL_DEMULTIPLEXER]     = { "signal_demultiplexer",    NULL,                                           0                                                         },
	[JDKSAVDECC_DESCRIPTOR_SIGNAL_MULTIPLEXER]       = { "signal_multiplexer",      NULL,                                           0                                                         },
	[JDKSAVDECC_DESCRIPTOR_SIGNAL_TRANSCODER]        = { "signal_transcoder",       NULL,                                           0                                                         },
	[JDKSAVDECC_DESCRIPTOR_CLOCK_DOMAIN]             = { "clock_domain",            avdecc_desc_parse_table_clock_domain,           ARRAY_SIZE(avdecc_desc_parse_table_clock_domain)          },
	[JDKSAVDECC_DESCRIPTOR_CONTROL_BLOCK]            = { "control_block",           NULL,                                           0                                                         },
};

static int assigned_string_data(const char *value, void *d)
{
	jdksavdecc_string_assign(d, value);

	return 0;
}

static int assigned_valiable_data(const char *value, void *d)
{
	uint32_t len = 0;
	uint32_t i = 0;
	uint32_t x = 0;
	uint8_t *data = d;

	len = (int)strlen(value);

	for (i = 0; i < len; i += 2) {
		sscanf((char *)(value + i), "%02x", &x);
		data[i / 2] = x;
	}
	return 0;
}

static int assigned_eui48_data(const char *value, void *d)
{
	jdksavdecc_eui48_init_from_cstr(d, value);

	return 0;
}

static int assigned_eui64_data(const char *value, void *d)
{
	jdksavdecc_eui64_init_from_cstr(d, value);

	return 0;
}

static int assigned_uint32_data(const char *value, void *d)
{
	char *e;
	uint32_t *data = d;

	errno = 0;
	*data = strtol(value, &e, 0);
	if (errno != 0) {
		perror("strtol");
		return -1;
	}

	if (e == value) {
		AVDECC_PRINTF1("%s: not a number\n", value);
		return -1;
	}

	return 0;
}

static void yaml_to_descriptor_entry(
	char *desc,
	char *key,
	char *value,
	struct avdecc_desc_parse_entry *p_table,
	int p_table_len)
{
	int i;

	for (i = 0; i < p_table_len; i++)
		if (!strcmp(key, p_table[i].name))
			p_table[i].parse(value, (void *)(desc + p_table[i].offset));
}

static int yaml_to_descriptor(
	int desc_type,
	struct descriptor_data *descriptor,
	char *key,
	char *value)
{
	struct parse_descriptor *p;

	if (!descriptor)
		return -1;

	if (!key)
		return -1;

	if (!value)
		return -1;

	p = &d_table[desc_type];

	switch (desc_type) {
	case JDKSAVDECC_DESCRIPTOR_ENTITY:
	case JDKSAVDECC_DESCRIPTOR_CONFIGURATION: /*check*/
	case JDKSAVDECC_DESCRIPTOR_VIDEO_UNIT:
	case JDKSAVDECC_DESCRIPTOR_JACK_INPUT:
	case JDKSAVDECC_DESCRIPTOR_JACK_OUTPUT:
	case JDKSAVDECC_DESCRIPTOR_AVB_INTERFACE:
	case JDKSAVDECC_DESCRIPTOR_CLOCK_SOURCE:
	case JDKSAVDECC_DESCRIPTOR_LOCALE:
	case JDKSAVDECC_DESCRIPTOR_STRINGS:
	case JDKSAVDECC_DESCRIPTOR_STREAM_PORT_INPUT:
	case JDKSAVDECC_DESCRIPTOR_STREAM_PORT_OUTPUT:
	case JDKSAVDECC_DESCRIPTOR_AUDIO_CLUSTER:
		yaml_to_descriptor_entry((void *)&descriptor->desc, key, value, p->entry, p->entry_size);
		break;
	case JDKSAVDECC_DESCRIPTOR_AUDIO_UNIT:
		if (!strcmp(key, "sampling_rates")) {
			if (descriptor->desc.audio_unit_desc.audio_unit.sampling_rates_count != 0) {
				descriptor->desc.audio_unit_desc.sampling_rates = calloc(descriptor->desc.audio_unit_desc.audio_unit.sampling_rates_count, sizeof(uint32_t));
				if (!descriptor->desc.audio_unit_desc.sampling_rates) {
					AVDECC_PRINTF1("Failed to allocate buffer.(sampling_rates)\n");
					return -1;
				}
				assigned_valiable_data(value, (void *)descriptor->desc.audio_unit_desc.sampling_rates);
			} else {
				AVDECC_PRINTF1("audio_uint->sampling_rates_count is zero\n");
				return -1;
			}
		} else {
			yaml_to_descriptor_entry((void *)&descriptor->desc.audio_unit_desc.audio_unit, key, value, p->entry, p->entry_size);
		}
		break;
	case JDKSAVDECC_DESCRIPTOR_STREAM_INPUT:
	case JDKSAVDECC_DESCRIPTOR_STREAM_OUTPUT:
		if (!strcmp(key, "formats")) {
			if (descriptor->desc.stream_desc.stream.number_of_formats != 0) {
				descriptor->desc.stream_desc.formats =
					calloc(descriptor->desc.stream_desc.stream.number_of_formats,
					       sizeof(uint64_t));
				if (!descriptor->desc.stream_desc.formats) {
					AVDECC_PRINTF1("Failed to allocate buffer.(formats)\n");
					return -1;
				}
				assigned_valiable_data(value, (void *)descriptor->desc.stream_desc.formats);
			} else {
				AVDECC_PRINTF1("stream_port.number_of_formats is zero\n");
				return -1;
			}
		} else {
			yaml_to_descriptor_entry((void *)&descriptor->desc.stream_desc.stream, key, value, p->entry, p->entry_size);
		}
		break;
	case JDKSAVDECC_DESCRIPTOR_VIDEO_CLUSTER:
		if (!strcmp(key, "supported_format_specifics")) {
			if (descriptor->desc.video_unit_cluster_desc.video_unit_cluster.supported_format_specifics_count != 0) {
				descriptor->desc.video_unit_cluster_desc.supported_format_specifics = calloc(descriptor->desc.video_unit_cluster_desc.video_unit_cluster.supported_format_specifics_count, sizeof(uint32_t));
				if (!descriptor->desc.video_unit_cluster_desc.supported_format_specifics) {
					AVDECC_PRINTF1("Failed to allocate buffer.(supported_format_specifics)\n");
					return -1;
				}
				assigned_valiable_data(value, (void *)descriptor->desc.video_unit_cluster_desc.supported_format_specifics);
			} else {
				AVDECC_PRINTF1("video_unit_cluster.supported_format_specifics_count is zero\n");
				return -1;
			}
		} else if (!strcmp(key, "supported_sampling_rates")) {
			if (descriptor->desc.video_unit_cluster_desc.video_unit_cluster.supported_sampling_rates_count != 0) {
				descriptor->desc.video_unit_cluster_desc.supported_sampling_rates = calloc(descriptor->desc.video_unit_cluster_desc.video_unit_cluster.supported_sampling_rates_count, sizeof(uint32_t));
				if (!descriptor->desc.video_unit_cluster_desc.supported_sampling_rates) {
					AVDECC_PRINTF1("Failed to allocate buffer.(supported_sampling_rates)\n");
					return -1;
				}
				assigned_valiable_data(value, (void *)descriptor->desc.video_unit_cluster_desc.supported_sampling_rates);
			} else {
				AVDECC_PRINTF1("video_unit_cluster.supported_sampling_rates_count is zero\n");
			}
		} else if (!strcmp(key, "supported_aspect_ratios")) {
			if (descriptor->desc.video_unit_cluster_desc.video_unit_cluster.supported_aspect_ratios_count != 0) {
				descriptor->desc.video_unit_cluster_desc.supported_aspect_ratios = calloc(descriptor->desc.video_unit_cluster_desc.video_unit_cluster.supported_aspect_ratios_count, sizeof(uint16_t));
				if (!descriptor->desc.video_unit_cluster_desc.supported_aspect_ratios) {
					AVDECC_PRINTF1("Failed to allocate buffer.(supported_aspect_ratios)\n");
					return -1;
				}
				assigned_valiable_data(value, (void *)descriptor->desc.video_unit_cluster_desc.supported_aspect_ratios);
			} else {
				AVDECC_PRINTF1("video_unit_cluster.supported_aspect_ratios_count is zero\n");
				return -1;
			}
		} else if (!strcmp(key, "supported_sizes")) {
			if (descriptor->desc.video_unit_cluster_desc.video_unit_cluster.supported_sizes_count != 0) {
				descriptor->desc.video_unit_cluster_desc.supported_sizes = calloc(descriptor->desc.video_unit_cluster_desc.video_unit_cluster.supported_sizes_count, sizeof(uint32_t));
				if (!descriptor->desc.video_unit_cluster_desc.supported_sizes) {
					AVDECC_PRINTF1("Failed to allocate buffer.(supported_sizes)\n");
					return -1;
				}
				assigned_valiable_data(value, (void *)descriptor->desc.video_unit_cluster_desc.supported_sizes);
			} else {
				AVDECC_PRINTF1("video_unit_cluster.supported_sizes_count is zero\n");
				return -1;
			}
		} else if (!strcmp(key, "supported_color_spaces")) {
			if (descriptor->desc.video_unit_cluster_desc.video_unit_cluster.supported_color_spaces_count != 0) {
				descriptor->desc.video_unit_cluster_desc.supported_color_spaces = calloc(descriptor->desc.video_unit_cluster_desc.video_unit_cluster.supported_color_spaces_count, sizeof(uint16_t));
				if (!descriptor->desc.video_unit_cluster_desc.supported_color_spaces) {
					AVDECC_PRINTF1("Failed to allocate buffer.(supported_color_spaces)\n");
					return -1;
				}
				assigned_valiable_data(value, (void *)descriptor->desc.video_unit_cluster_desc.supported_color_spaces);
			} else {
				AVDECC_PRINTF1("video_unit_cluster.supported_color_spaces_count is zero\n");
				return -1;
			}
		} else {
			yaml_to_descriptor_entry((void *)&descriptor->desc, key, value, p->entry, p->entry_size);
		}
		break;
	case JDKSAVDECC_DESCRIPTOR_AUDIO_MAP:
		if (!strcmp(key, "mappings")) {
			if (descriptor->desc.audio_map_desc.audio_map.number_of_mappings != 0) {
				descriptor->desc.audio_map_desc.mappings = calloc(descriptor->desc.audio_map_desc.audio_map.number_of_mappings, sizeof(struct jdksavdecc_audio_mapping));
				if (!descriptor->desc.audio_map_desc.mappings) {
					AVDECC_PRINTF1("Failed to allocate buffer.(mappings)\n");
					return -1;
				}
				assigned_valiable_data(value, (void *)descriptor->desc.audio_map_desc.mappings);
			} else {
				AVDECC_PRINTF1("number_of_mappings is zero\n");
				return -1;
			}
		} else {
			yaml_to_descriptor_entry((void *)&descriptor->desc, key, value, p->entry, p->entry_size);
		}
		break;
	case JDKSAVDECC_DESCRIPTOR_VIDEO_MAP:
		if (!strcmp(key, "mappings")) {
			if (descriptor->desc.video_unit_map_desc.video_unit_map.number_of_mappings != 0) {
				descriptor->desc.video_unit_map_desc.mappings = calloc(descriptor->desc.video_unit_map_desc.video_unit_map.number_of_mappings, sizeof(struct jdksavdecc_video_mapping));
				if (!descriptor->desc.video_unit_map_desc.mappings) {
					AVDECC_PRINTF1("Failed to allocate buffer.(mappings)\n");
					return -1;
				}
				assigned_valiable_data(value, (void *)descriptor->desc.video_unit_map_desc.mappings);
			} else {
				AVDECC_PRINTF1("number_of_mappings is zero\n");
				return -1;
			}
		} else {
			yaml_to_descriptor_entry((void *)&descriptor->desc, key, value, p->entry, p->entry_size);
		}
		break;
	case JDKSAVDECC_DESCRIPTOR_CLOCK_DOMAIN:
		if (!strcmp(key, "clock_sources")) {
			if (descriptor->desc.clock_domain_desc.clock_domain.clock_sources_count != 0) {
				descriptor->desc.clock_domain_desc.clock_sources = calloc(descriptor->desc.clock_domain_desc.clock_domain.clock_sources_count, sizeof(uint16_t));
				if (!descriptor->desc.clock_domain_desc.clock_sources) {
					AVDECC_PRINTF1("Failed to allocate buffer.(sources)\n");
					return -1;
				}
				assigned_valiable_data(value, (void *)descriptor->desc.clock_domain_desc.clock_sources);
			} else {
				AVDECC_PRINTF1("clock_domain->number_of_mappings is zero\n");
				return -1;
			}
		} else {
			yaml_to_descriptor_entry((void *)&descriptor->desc, key, value, p->entry, p->entry_size);
		}
		break;
	default:
		/* not support */
		return -1;
	}
	return 0;
}

static const char *yaml_token_type_to_string(yaml_token_type_t v)
{
	switch (v) {
	case YAML_NO_TOKEN:                     return "NO_TOKEN";
	case YAML_STREAM_START_TOKEN:           return "STREAM_START";
	case YAML_STREAM_END_TOKEN:             return "STREAM_END";
	case YAML_VERSION_DIRECTIVE_TOKEN:      return "VERSION_DIRECTIVE";
	case YAML_TAG_DIRECTIVE_TOKEN:          return "TAG_DIRECTIVE";
	case YAML_DOCUMENT_START_TOKEN:         return "DOCUMENT_START";
	case YAML_DOCUMENT_END_TOKEN:           return "DOCUMENT_END";
	case YAML_BLOCK_SEQUENCE_START_TOKEN:   return "BLOCK_SEQUENCE_START";
	case YAML_BLOCK_MAPPING_START_TOKEN:    return "BLOCK_MAPPING_START";
	case YAML_BLOCK_END_TOKEN:              return "BLOCK_END";
	case YAML_FLOW_SEQUENCE_START_TOKEN:    return "FLOW_SEQUENCE_START";
	case YAML_FLOW_SEQUENCE_END_TOKEN:      return "FLOW_SEQUENCE_END";
	case YAML_FLOW_MAPPING_START_TOKEN:     return "FLOW_MAPPING_START";
	case YAML_FLOW_MAPPING_END_TOKEN:       return "FLOW_MAPPING_END";
	case YAML_BLOCK_ENTRY_TOKEN:            return "BLOCK_ENTRY";
	case YAML_FLOW_ENTRY_TOKEN:             return "FLOW_ENTRY";
	case YAML_KEY_TOKEN:                    return "KEY";
	case YAML_VALUE_TOKEN:                  return "VALUE";
	case YAML_ALIAS_TOKEN:                  return "ALIAS";
	case YAML_ANCHOR_TOKEN:                 return "ANCHOR";
	case YAML_TAG_TOKEN:                    return "TAG";
	case YAML_SCALAR_TOKEN:                 return "SCALAR";
	default:                                return "?";
	}
}

static bool is_support_descriptor_type(uint16_t desc_type)
{
	switch (desc_type) {
	case JDKSAVDECC_DESCRIPTOR_AUDIO_UNIT:
	case JDKSAVDECC_DESCRIPTOR_VIDEO_UNIT:
	case JDKSAVDECC_DESCRIPTOR_STREAM_INPUT:
	case JDKSAVDECC_DESCRIPTOR_STREAM_OUTPUT:
	case JDKSAVDECC_DESCRIPTOR_JACK_INPUT:
	case JDKSAVDECC_DESCRIPTOR_JACK_OUTPUT:
	case JDKSAVDECC_DESCRIPTOR_AVB_INTERFACE:
	case JDKSAVDECC_DESCRIPTOR_CLOCK_SOURCE:
	case JDKSAVDECC_DESCRIPTOR_LOCALE:
	case JDKSAVDECC_DESCRIPTOR_STRINGS:
	case JDKSAVDECC_DESCRIPTOR_STREAM_PORT_INPUT:
	case JDKSAVDECC_DESCRIPTOR_STREAM_PORT_OUTPUT:
	case JDKSAVDECC_DESCRIPTOR_AUDIO_CLUSTER:
	case JDKSAVDECC_DESCRIPTOR_VIDEO_CLUSTER:
	case JDKSAVDECC_DESCRIPTOR_AUDIO_MAP:
	case JDKSAVDECC_DESCRIPTOR_VIDEO_MAP:
	case JDKSAVDECC_DESCRIPTOR_CLOCK_DOMAIN:
		return true;
	default:
		return false;
	}
}

static struct descriptor_data *add_new_descriptor(struct descriptor_data *head, int desc_type)
{
	struct descriptor_data *p = head;
	struct descriptor_data *new_desc;

	new_desc = calloc(1, sizeof(*new_desc));
	if (!new_desc) {
		AVDECC_PRINTF1("could not allocat new_config\n");
		return NULL;
	}

	new_desc->desc.entity_desc.entity.descriptor_type = desc_type;
	new_desc->next = NULL;

	if (head == NULL)
		return new_desc;

	while (p->next != NULL)
		p = p->next;
	p->next = new_desc;

	return head;
}

static int release_descriptor_data(struct descriptor_data *head)
{
	struct descriptor_data *p = NULL;
	struct descriptor_data *next = NULL;

	if (!head)
		return -1;

	for (p = head; p != NULL; p = next) {
		next = p->next;
		free(p);
	}

	return 0;
}

static struct configuration_list *add_new_configuration(struct configuration_list *head)
{
	struct configuration_list *p = head;
	struct configuration_list *new_config;

	new_config = calloc(1, sizeof(*new_config));
	if (!new_config) {
		AVDECC_PRINTF1("could not allocat new_config\n");
		return NULL;
	}

	new_config->next = NULL;

	if (head == NULL)
		return new_config;

	while (p->next != NULL)
		p = p->next;
	p->next = new_config;

	return head;
}

static int release_configuration_list(struct configuration_list *head)
{
	struct configuration_list *p = NULL;
	struct configuration_list *next = NULL;

	if (!head)
		return -1;

	for (p = head; p != NULL; p = next) {
		next = p->next;
		free(p);
	}

	return 0;
}

enum PARSE_STATUS {
	PARSE_IDLE,
	PARSE_START,
	PARSE_ENTITY_START,
	PARSE_ENTITY_FOUND,
	PARSE_ENTITY_SEQUENCE,
	PARSE_ENTITY,
	PARSE_CONFIGURATION_FOUND,
	PARSE_CONFIGURATION_SEQUENCE,
	PARSE_CONFIGURATION,
	PARSE_DESCRIPTORS_FOUND,
	PARSE_DESCRIPTORS_START,
	PARSE_DESCRIPTOR_FOUND,
	PARSE_DESCRIPTOR_SEQUENCE,
	PARSE_DESCRIPTOR,
};

struct device_entity *read_device_entity(const char *filename)
{
	FILE *fp;
	yaml_parser_t parser;
	yaml_token_t token;
	int key_value_state = 0;  /*  state 0 = key, 1 = value */
	int state = PARSE_IDLE;
	struct configuration_list *latest_configuration = NULL;
	struct descriptor_data    *latest_descriptor = NULL;
	int flag = 0;
	char tk_key[1024];
	char tk_value[1024];
	struct device_entity *dev_e;
	int i;
	int rc = 0;
	int error = 0;
	struct parse_descriptor *p = NULL;

	AVDECC_PRINTF2("read_device_enitity() started.\n");

	if (!filename) {
		AVDECC_PRINTF1("filename is NULL.\n");
		return NULL;
	}

	fp = fopen(filename, "r");
	if (!fp) {
		AVDECC_PRINTF1("Failed to open file.\n");
		return NULL;
	}

	dev_e = calloc(1, sizeof(struct device_entity));
	if (!dev_e) {
		AVDECC_PRINTF1("Failed to allocate buffer(dev_e).\n");
		fclose(fp);
		return NULL;
	}

	if (!yaml_parser_initialize(&parser)) {
		AVDECC_PRINTF1("Failed to initialize parser.\n");
		fclose(fp);
		free(dev_e);
		return NULL;
	}

	yaml_parser_set_input_file(&parser, fp);

	do {
		yaml_parser_scan(&parser, &token);

		AVDECC_PRINTF3("YAML: token.type:%-2d flag:%d key_value_state:%d token.type_s:%s\n",
			       token.type,
			       flag,
			       key_value_state,
			       yaml_token_type_to_string(token.type));

		switch (token.type) {
		case YAML_STREAM_START_TOKEN:
			state = PARSE_START;
			break;
		case YAML_STREAM_END_TOKEN:
			break;
		case YAML_BLOCK_SEQUENCE_START_TOKEN:
			switch (state) {
			case PARSE_ENTITY_FOUND:
				state = PARSE_ENTITY_SEQUENCE;
				flag = JDKSAVDECC_DESCRIPTOR_ENTITY;
				break;
			case PARSE_CONFIGURATION_FOUND:
				state = PARSE_CONFIGURATION_SEQUENCE;
				flag = JDKSAVDECC_DESCRIPTOR_CONFIGURATION;
				break;
			case PARSE_DESCRIPTOR_FOUND:
				state = PARSE_DESCRIPTOR_SEQUENCE;
				break;
			default:
				break;
			}
			break;
		case YAML_BLOCK_ENTRY_TOKEN:
			break;
		case YAML_BLOCK_MAPPING_START_TOKEN:
			switch (state) {
			case PARSE_START:
				state = PARSE_ENTITY_START;
				break;
			case PARSE_ENTITY_SEQUENCE:
				state = PARSE_ENTITY;
				break;
			case PARSE_CONFIGURATION_SEQUENCE:
				state = PARSE_CONFIGURATION;
				dev_e->configuration_list = add_new_configuration(dev_e->configuration_list);
				if (!dev_e->configuration_list) {
					AVDECC_PRINTF1("Failed to allocate configuration list\n");
					goto end;
				}
				latest_configuration = dev_e->configuration_list;
				while (latest_configuration->next != NULL)
					latest_configuration = latest_configuration->next;
				flag = JDKSAVDECC_DESCRIPTOR_CONFIGURATION;
				break;
			case PARSE_DESCRIPTORS_FOUND:
				state = PARSE_DESCRIPTORS_START;
				break;
			case PARSE_DESCRIPTOR_SEQUENCE:
				if (!dev_e->configuration_list) {
					AVDECC_PRINTF1("ERROR: configuration_list is NULL\n");
					error = 1;
					goto end;
				}
				latest_configuration->descriptor = add_new_descriptor(latest_configuration->descriptor, flag);
				if (!latest_configuration->descriptor) {
					AVDECC_PRINTF1("Failed to allocate descriptor data is NULL\n");
					goto end;
				}
				latest_descriptor = latest_configuration->descriptor;
				while (latest_descriptor->next != NULL)
					latest_descriptor = latest_descriptor->next;
				state = PARSE_DESCRIPTOR;
				break;
			default:
				break;
			}
			break;
		case YAML_BLOCK_END_TOKEN:
			switch (state) {
			case PARSE_DESCRIPTOR:
				state = PARSE_DESCRIPTOR_SEQUENCE;
				break;
			case PARSE_DESCRIPTOR_SEQUENCE:
				state = PARSE_DESCRIPTORS_START;
				break;
			case PARSE_DESCRIPTORS_START:
				state = PARSE_CONFIGURATION;
				break;
			case PARSE_CONFIGURATION:
				state = PARSE_CONFIGURATION_SEQUENCE;
				break;
			case PARSE_CONFIGURATION_SEQUENCE:
				state = PARSE_ENTITY;
				break;
			case PARSE_ENTITY:
				state = PARSE_ENTITY_SEQUENCE;
				break;
			case PARSE_ENTITY_SEQUENCE:
				state = PARSE_ENTITY_START;
				break;
			case PARSE_ENTITY_START:
				state = PARSE_START;
			default:
				break;
			}
			break;
		case YAML_KEY_TOKEN:
			key_value_state = 0;
			break;
		case YAML_VALUE_TOKEN:
			key_value_state = 1;
			break;
		case YAML_SCALAR_TOKEN:
			if (key_value_state) { /* value */
				memset(tk_value, 0, sizeof(tk_value));
				strncpy(tk_value,
					(char *)token.data.scalar.value,
					token.data.scalar.length);

				AVDECC_PRINTF3("value = %s\n", tk_value);

				switch (state) {
				case PARSE_ENTITY:
					rc = yaml_to_descriptor(flag, &dev_e->entity, tk_key, tk_value);
					break;
				case PARSE_CONFIGURATION:
					rc = yaml_to_descriptor(flag, &latest_configuration->configuration, tk_key, tk_value);
					break;
				case PARSE_DESCRIPTOR:
					rc = yaml_to_descriptor(flag, latest_descriptor, tk_key, tk_value);
					break;
				default:
					break;
				}

				if (rc < 0) {
					AVDECC_PRINTF1("Error in yaml_to_descriptor(%s type:%d).\n",
							(state == PARSE_ENTITY) ? "entity" :
							(state == PARSE_CONFIGURATION) ? "configuration" : "descriptor", flag);
					error = 1;
					goto end;
				}
			} else { /* key */
				memset(tk_key, 0, sizeof(tk_key));
				strncpy(tk_key,
					(char *)token.data.scalar.value,
					token.data.scalar.length);

				AVDECC_PRINTF3("key = %s\n", tk_key);

				switch (state) {
				case PARSE_ENTITY_START:
					if (!strcasecmp(tk_key, "entity")) {
						state = PARSE_ENTITY_FOUND;
					}
					break;
				case PARSE_ENTITY:
					if (!strcasecmp(tk_key, "configurations")) {
						state = PARSE_CONFIGURATION_FOUND;
					}
					break;
				case PARSE_CONFIGURATION:
					if (!strcasecmp(tk_key, "descriptors")) {
						state = PARSE_DESCRIPTORS_FOUND;
					}
					break;
				case PARSE_DESCRIPTORS_START:
					for (i = 0; i < JDKSAVDECC_NUM_DESCRIPTOR_TYPES; i++) {
						p = &d_table[i];
						if (strcasecmp(tk_key, p->name) == 0) {
							flag = i;
							state = PARSE_DESCRIPTOR_FOUND;
							break;
						}
					}
					if (i == JDKSAVDECC_NUM_DESCRIPTOR_TYPES)
						goto end;
					break;
				default:
					break;
				}
			}
			break;
		default:
			break;
		}

		if (token.type != YAML_STREAM_END_TOKEN)
			yaml_token_delete(&token);
	} while (token.type != YAML_STREAM_END_TOKEN);

end:
	yaml_token_delete(&token);
	yaml_parser_delete(&parser);
	fclose(fp);

	if (error) {
		AVDECC_PRINTF1("Error in read_device_enitity().\n");
		release_device_entity(dev_e);
		return NULL;
	}

	AVDECC_PRINTF2("read_device_enitity() was end.\n");

	return dev_e;
}

int release_device_entity(struct device_entity *dev_e)
{
	struct configuration_list *config_list = NULL;

	if (!dev_e)
		return -1;

	for (config_list = dev_e->configuration_list; config_list != NULL; config_list = config_list->next)
		release_descriptor_data(config_list->descriptor);

	release_configuration_list(dev_e->configuration_list);

	free(dev_e);

	return 0;
}

int setup_descriptor_data(struct device_entity *dev_e, uint8_t *mac_addr)
{
	int i;
	struct jdksavdecc_descriptor_entity *entity;
	struct jdksavdecc_descriptor_configuration *configuration;
	struct configuration_list *config_list = NULL;
	struct descriptor_data *desc_data = NULL;
	int configuration_count = 0;
	int desc_counts_count = 0;
	int desc_nums[JDKSAVDECC_NUM_DESCRIPTOR_TYPES];
	struct descriptor_counts_fmt *descriptor_counts;
	uint16_t desc_type;
	uint16_t desc_index;

	if (!dev_e)
		return -1;

	if (!mac_addr)
		return -1;

	entity = &dev_e->entity.desc.entity_desc.entity;
	entity->descriptor_type = 0x0;
	entity->descriptor_index = 0x0;

	for (config_list = dev_e->configuration_list; config_list != NULL; config_list = config_list->next, configuration_count++) {
		/* clear descriptor numbers */
		memset(desc_nums, 0, sizeof(desc_nums));
		desc_counts_count = 0;

		for (desc_data = config_list->descriptor; desc_data != NULL; desc_data = desc_data->next) {
			desc_type = desc_data->desc.entity_desc.entity.descriptor_type;

			if (!is_support_descriptor_type(desc_type))
				continue; /* not support descriptor */

			desc_index = desc_nums[desc_type];
			desc_nums[desc_type] += 1;

			if (desc_index == 0)
				desc_counts_count += 1;

			switch (desc_type) {
			case JDKSAVDECC_DESCRIPTOR_AUDIO_UNIT:
				desc_data->desc.audio_unit_desc.audio_unit.descriptor_index = desc_index;
				desc_data->desc.audio_unit_desc.audio_unit.sampling_rates_offset = JDKSAVDECC_DESCRIPTOR_AUDIO_UNIT_OFFSET_SAMPLING_RATES;
				break;
			case JDKSAVDECC_DESCRIPTOR_VIDEO_UNIT:
				desc_data->desc.video_unit_desc.video_unit.descriptor_index = desc_index;
				break;
			case JDKSAVDECC_DESCRIPTOR_STREAM_INPUT:
				if (config_list->configuration.desc.configuration_desc.configuration.descriptor_index == entity->current_configuration)
					entity->listener_stream_sinks++;

				desc_data->desc.stream_desc.stream.descriptor_index = desc_index;
				desc_data->desc.stream_desc.stream.formats_offset = JDKSAVDECC_DESCRIPTOR_STREAM_OFFSET_FORMATS;
				break;
			case JDKSAVDECC_DESCRIPTOR_STREAM_OUTPUT:
				if (config_list->configuration.desc.configuration_desc.configuration.descriptor_index == entity->current_configuration)
					entity->talker_stream_sources++;

				desc_data->desc.stream_desc.stream.descriptor_index = desc_index;
				desc_data->desc.stream_desc.stream.formats_offset = JDKSAVDECC_DESCRIPTOR_STREAM_OFFSET_FORMATS;
				break;
			case JDKSAVDECC_DESCRIPTOR_JACK_INPUT:
				desc_data->desc.jack_desc.jack.descriptor_index = desc_index;
				break;
			case JDKSAVDECC_DESCRIPTOR_JACK_OUTPUT:
				desc_data->desc.jack_desc.jack.descriptor_index = desc_index;
				break;
			case JDKSAVDECC_DESCRIPTOR_AVB_INTERFACE:
				desc_data->desc.avb_interface_desc.avb_interface.descriptor_index = desc_index;
				memcpy(desc_data->desc.avb_interface_desc.avb_interface.mac_address.value, mac_addr, ETH_ALEN);
				break;
			case JDKSAVDECC_DESCRIPTOR_CLOCK_SOURCE:
				desc_data->desc.clock_source_desc.clock_source.descriptor_index = desc_index;
				break;
			case JDKSAVDECC_DESCRIPTOR_LOCALE:
				desc_data->desc.locale_desc.locale.descriptor_index = desc_index;
				break;
			case JDKSAVDECC_DESCRIPTOR_STRINGS:
				desc_data->desc.strings_desc.strings.descriptor_index = desc_index;
				break;
			case JDKSAVDECC_DESCRIPTOR_STREAM_PORT_INPUT:
				desc_data->desc.stream_port_desc.stream_port.descriptor_index = desc_index;
				break;
			case JDKSAVDECC_DESCRIPTOR_STREAM_PORT_OUTPUT:
				desc_data->desc.stream_port_desc.stream_port.descriptor_index = desc_index;
				break;
			case JDKSAVDECC_DESCRIPTOR_AUDIO_CLUSTER:
				desc_data->desc.audio_cluster_desc.audio_cluster.descriptor_index = desc_index;
				break;
			case JDKSAVDECC_DESCRIPTOR_VIDEO_CLUSTER:
				desc_data->desc.video_unit_cluster_desc.video_unit_cluster.descriptor_index = desc_index;
				break;
			case JDKSAVDECC_DESCRIPTOR_AUDIO_MAP:
				desc_data->desc.audio_map_desc.audio_map.descriptor_index = desc_index;
				desc_data->desc.audio_map_desc.audio_map.mappings_offset = JDKSAVDECC_DESCRIPTOR_AUDIO_MAP_OFFSET_MAPPINGS;
				break;
			case JDKSAVDECC_DESCRIPTOR_VIDEO_MAP:
				desc_data->desc.video_unit_map_desc.video_unit_map.descriptor_index = desc_index;
				desc_data->desc.video_unit_map_desc.video_unit_map.mappings_offset = JDKSAVDECC_DESCRIPTOR_VIDEO_MAP_OFFSET_MAPPINGS;
				break;
			case JDKSAVDECC_DESCRIPTOR_CLOCK_DOMAIN:
				desc_data->desc.clock_domain_desc.clock_domain.descriptor_index = desc_index;
				desc_data->desc.clock_domain_desc.clock_domain.clock_sources_offset = JDKSAVDECC_DESCRIPTOR_CLOCK_DOMAIN_OFFSET_CLOCK_SOURCES;
				break;

			default:
				break;
			}
		}

		descriptor_counts = calloc(desc_counts_count, sizeof(*descriptor_counts));
		if (!descriptor_counts)
			return -1;

		configuration = &config_list->configuration.desc.configuration_desc.configuration;
		configuration->descriptor_type = JDKSAVDECC_DESCRIPTOR_CONFIGURATION;
		configuration->descriptor_counts_offset = JDKSAVDECC_DESCRIPTOR_CONFIGURATION_OFFSET_DESCRIPTOR_COUNTS;
		configuration->descriptor_index = configuration_count;
		configuration->descriptor_counts_count = desc_counts_count;
		config_list->configuration.desc.configuration_desc.descriptor_counts = descriptor_counts;

		for (i = 0; i < JDKSAVDECC_NUM_DESCRIPTOR_TYPES; i++) {
			if (desc_nums[i] != 0) {
				descriptor_counts->descriptor_type = i;
				descriptor_counts->count = desc_nums[i];
				descriptor_counts++;
			}
		}

		for (desc_data = config_list->descriptor; desc_data != NULL; desc_data = desc_data->next)
			if (desc_data->desc.entity_desc.entity.descriptor_type == JDKSAVDECC_DESCRIPTOR_LOCALE)
				desc_data->desc.locale_desc.locale.number_of_strings = desc_nums[JDKSAVDECC_DESCRIPTOR_STRINGS]++;
	}

	entity->configurations_count = configuration_count;

	return 0;
}
