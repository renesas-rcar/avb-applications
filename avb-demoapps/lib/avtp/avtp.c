/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#include <string.h>

#include "avtp.h"

#define LIBVERSION "0.3"

/*
 * Structure
 */
struct ether_hdr {
	uint8_t  dest[6];
	uint8_t  source[6];
	uint16_t ethtype;
	uint8_t payload[0];
} __attribute__ ((packed));

struct ieee8021q_hdr {
	uint8_t  dest[6];
	uint8_t  source[6];
	uint16_t tpid;
	uint16_t tci;
	uint16_t ethtype;
	uint8_t payload[0];
} __attribute__ ((packed));

/* P1722/D16 5.4.3 AVTPDU common stream header */
#if __BYTE_ORDER == __BIG_ENDIAN
struct avtp_stream_hdr {
	uint8_t subtype;
	uint8_t sv:1;
	uint8_t version:3;
	uint8_t mr:1;
	uint8_t f_s_d:2;
	uint8_t tv:1;
	uint8_t sequence_num;
	uint8_t format_specific_data_1:7;
	uint8_t tu:1;
	uint64_t stream_id;
	uint32_t avtp_timestamp;
	uint32_t format_specific_data_2;
	uint16_t stream_data_length;
	uint16_t format_specific_data_3;
	uint8_t payload[0];
} __attribute__((packed));
#else
struct avtp_stream_hdr {
	uint8_t subtype;
	uint8_t tv:1;
	uint8_t f_s_d:2;
	uint8_t mr:1;
	uint8_t version:3;
	uint8_t sv:1;
	uint8_t sequence_num;
	uint8_t tu:1;
	uint8_t format_specific_data_1:7;
	uint64_t stream_id;
	uint32_t avtp_timestamp;
	uint32_t format_specific_data_2;
	uint16_t stream_data_length;
	uint16_t format_specific_data_3;
	uint8_t payload[0];
} __attribute__((packed));
#endif

#if __BYTE_ORDER == __BIG_ENDIAN
struct avtp_cvf_hdr {
	uint8_t  subtype;
	uint8_t  sv:1;
	uint8_t  version:3;
	uint8_t  mr:1;
	uint8_t  reserved0:2;
	uint8_t  tv:1;
	uint8_t  sequence_num;
	uint8_t  reserved1:7;
	uint8_t  tu:1;
	uint64_t stream_id;
	uint32_t avtp_timestamp;
	uint8_t  format;
	uint8_t  format_subtype;
	uint16_t reserved2;
	uint16_t stream_data_length;
	uint8_t  M:1;
	uint8_t  evt:4;
	uint8_t  reserved3:3;
	uint8_t  reserved4;
	uint8_t  payload[0];
} __attribute__((packed));
#else
struct avtp_cvf_hdr {
	uint8_t  subtype;
	uint8_t  tv:1;
	uint8_t  reserved0:2;
	uint8_t  mr:1;
	uint8_t  version:3;
	uint8_t  sv:1;
	uint8_t  sequence_num;
	uint8_t  tu:1;
	uint8_t  reserved1:7;
	uint64_t stream_id;
	uint32_t avtp_timestamp;
	uint8_t  format;
	uint8_t  format_subtype;
	uint16_t reserved2;
	uint16_t stream_data_length;
	uint8_t  reserved3:3;
	uint8_t  evt:4;
	uint8_t  M:1;
	uint8_t  reserved4;
	uint8_t  payload[0];
} __attribute__((packed));
#endif

/* AVTP Streame common header */
static const struct avtp_stream_hdr avtp_stream_hdr_tmpl = {
	.subtype                = 0,
	.sv                     = 1,
	.version                = 0,
	.mr                     = 0,
	.f_s_d                  = 0,
	.tv                     = 1,
	.sequence_num           = 0,
	.format_specific_data_1	= 0,
	.tu                     = 0,
	.stream_id              = 0,
	.avtp_timestamp         = 0,
	.format_specific_data_2 = 0,
	.stream_data_length     = 0,
	.format_specific_data_3 = 0,
};
void copy_avtp_stream_template(void *data)
{
	memcpy(data, &avtp_stream_hdr_tmpl, sizeof(avtp_stream_hdr_tmpl));
}

/* AVTP Video (CVF) Experimental header */
static const struct avtp_cvf_hdr avtp_cvf_experimental_hdr_tmpl = {
	.subtype               = AVTP_SUBTYPE_CVF,
	.sv                    = 1,
	.version               = 0,
	.mr                    = 0,
	.reserved0             = 0,
	.tv                    = 1,
	.sequence_num          = 0,
	.reserved1             = 0,
	.tu                    = 0,
	.stream_id             = 0,
	.avtp_timestamp        = 0,
	.format                = AVTP_CVF_FORMAT_EXPERIMENTAL,
	.format_subtype        = 0,
	.reserved2             = 0,
	.stream_data_length    = 0,
	.reserved3             = 0,
	.M                     = 0,
	.evt                   = 0,
	.reserved4             = 0,
};
void copy_avtp_cvf_experimental_template(void *data)
{
	memcpy(data + AVTP_OFFSET, &avtp_cvf_experimental_hdr_tmpl, sizeof(avtp_cvf_experimental_hdr_tmpl));
}

