/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#ifndef __AVTP_H__
#define __AVTP_H__

#include <stdint.h>
#include <arpa/inet.h>

#define ETH_P_1722 (0x22F0)

#ifndef AVTP_OFFSET
/* Ethernet frame header length (DA + SA + Qtag + EthType) */
#define AVTP_OFFSET (18)
#endif

#define AVTP_PAYLOAD_OFFSET (24 + AVTP_OFFSET)
#define AVTP_CVF_PAYLOAD_OFFSET (AVTP_PAYLOAD_OFFSET)

#define AVTP_STREAMID_SIZE (8)

#define AVTP_SEQUENCE_NUM_MAX (255)
#define AVTP_UNIQUE_ID_MAX  (65535)

#define DEF_GETTER_UINT8(name, offset) \
	static inline uint8_t get_##name(void *data) \
	{ \
		return *((uint8_t *)(data + offset)); \
	}

#define DEF_SETTER_UINT8(name, offset) \
	static inline void set_##name(void *data, uint8_t value) \
	{ \
		*((uint8_t *)(data + offset)) = value; \
	}

#define DEF_ACCESSER_UINT8(name, offset) \
	DEF_GETTER_UINT8(name, offset) \
	DEF_SETTER_UINT8(name, offset) \

#define DEF_GETTER_UINT16(name, offset) \
	static inline uint16_t get_##name(void *data) \
	{ \
		return htons(*((uint16_t *)(data + offset))); \
	}

#define DEF_SETTER_UINT16(name, offset) \
	static inline void set_##name(void *data, uint16_t value) \
	{ \
		*((uint16_t *)(data + offset)) = htons(value); \
	}

#define DEF_ACCESSER_UINT16(name, offset) \
	DEF_GETTER_UINT16(name, offset) \
	DEF_SETTER_UINT16(name, offset) \

#define DEF_GETTER_UINT32(name, offset) \
	static inline uint32_t get_##name(void *data) \
	{ \
		return htonl(*((uint32_t *)(data + offset))); \
	}

#define DEF_SETTER_UINT32(name, offset) \
	static inline void set_##name(void *data, uint32_t value) \
	{ \
		*((uint32_t *)(data + offset)) = htonl(value); \
	}

#define DEF_ACCESSER_UINT32(name, offset) \
	DEF_GETTER_UINT32(name, offset) \
	DEF_SETTER_UINT32(name, offset) \

#define DEF_AVTP_GETTER_UINT8(name, offset) \
	DEF_GETTER_UINT8(avtp_##name, offset + AVTP_OFFSET)
#define DEF_AVTP_SETTER_UINT8(name, offset) \
	DEF_SETTER_UINT8(avtp_##name, offset + AVTP_OFFSET)
#define DEF_AVTP_ACCESSER_UINT8(name, offset) \
	DEF_ACCESSER_UINT8(avtp_##name, offset + AVTP_OFFSET)
#define DEF_AVTP_GETTER_UINT16(name, offset) \
	DEF_GETTER_UINT16(avtp_##name, offset + AVTP_OFFSET)
#define DEF_AVTP_SETTER_UINT16(name, offset) \
	DEF_SETTER_UINT16(avtp_##name, offset + AVTP_OFFSET)
#define DEF_AVTP_ACCESSER_UINT16(name, offset) \
	DEF_ACCESSER_UINT16(avtp_##name, offset + AVTP_OFFSET)
#define DEF_AVTP_GETTER_UINT32(name, offset) \
	DEF_GETTER_UINT32(avtp_##name, offset + AVTP_OFFSET)
#define DEF_AVTP_SETTER_UINT32(name, offset) \
	DEF_SETTER_UINT32(avtp_##name, offset + AVTP_OFFSET)
#define DEF_AVTP_ACCESSER_UINT32(name, offset) \
	DEF_ACCESSER_UINT32(avtp_##name, offset + AVTP_OFFSET)

/* P1722/D16 Table 6. AVTP stream data subtype values */
enum AVTP_SUBTYPE {
	AVTP_SUBTYPE_61883_IIDC  = 0x00, /* IEC 61883/IIDC Format */
	AVTP_SUBTYPE_MMA_STREAM  = 0x01, /* MMA Streams */
	AVTP_SUBTYPE_AAF         = 0x02, /* AVTP Audio Format */
	AVTP_SUBTYPE_CVF         = 0x03, /* Compressed Video Format */
	AVTP_SUBTYPE_CRF         = 0x04, /* Clock Reference Format */
	AVTP_SUBTYPE_TSCF        = 0x05, /* Time Synchronous Control Format */
	AVTP_SUBTYPE_SVF         = 0x06, /* SDI Video Format */
	AVTP_SUBTYPE_RVF         = 0x07, /* Raw Video Format */
	/* 0x08-0x6D Reserved */
	AVTP_SUBTYPE_AEF_CONTINUOUS = 0x6E, /* AES Encrypted Format Stream */
	AVTP_SUBTYPE_VSF_STREAM  = 0x6F, /* Vender Specific Foramt Stream */
	/* 0x70-0x7E Reserved */
	AVTP_SUBTYPE_EF_STREAM   = 0x7F, /* Experimental Foramt Stream */
	/* 0x80-0x81 Reserved */
	AVTP_SUBTYPE_NTSCF       = 0x82, /* Non Time Synchronous Control Format */
	/* 0x83-0xED Reserved */
	AVTP_SUBTYPE_ESCF        = 0xEC, /* ECC Signed Control Format */
	AVTP_SUBTYPE_EECF        = 0xED, /* ECC Encrypted Control Format */
	AVTP_SUBTYPE_AEF_DISCRETE = 0xEE, /* AES Encrypted Format Discrete */
	/* 0xEF-0xF9 Reserved */
	AVTP_SUBTYPE_ADP         = 0xFA, /* AVDECC Discovery Protocol */
	AVTP_SUBTYPE_AECP        = 0xFB, /* AVDECC Enumeration and Control Protocol */
	AVTP_SUBTYPE_ACMP        = 0xFC, /* AVDECC Connection Management Protocol */
	/* 0xFD Reserved */
	AVTP_SUBTYPE_MAAP        = 0xFE, /* MAAP Protocol */
	AVTP_SUBTYPE_EF_CONTROL  = 0xFF, /* Experimental Format Control */
};

/* P1722/D16 Table19. Compressed Video format field */
enum AVTP_CVF_FORMAT {
	AVTP_CVF_FORMAT_RFC          = 2,
	AVTP_CVF_FORMAT_EXPERIMENTAL = 0xff, /* P1722a/D5 */
};

/**
 * Accessor - IEEE802.1Q
 */
DEF_ACCESSER_UINT16(ieee8021q_tpid, 12);
DEF_ACCESSER_UINT16(ieee8021q_tci, 14);
DEF_ACCESSER_UINT16(ieee8021q_ethtype, 16);

static inline void get_ieee8021q_dest(void *data, uint8_t value[6])
{
	value[0] = *((uint8_t *)(data + 0));
	value[1] = *((uint8_t *)(data + 1));
	value[2] = *((uint8_t *)(data + 2));
	value[3] = *((uint8_t *)(data + 3));
	value[4] = *((uint8_t *)(data + 4));
	value[5] = *((uint8_t *)(data + 5));
}

static inline void set_ieee8021q_dest(void *data, uint8_t value[6])
{
	*((uint8_t *)(data + 0)) = value[0];
	*((uint8_t *)(data + 1)) = value[1];
	*((uint8_t *)(data + 2)) = value[2];
	*((uint8_t *)(data + 3)) = value[3];
	*((uint8_t *)(data + 4)) = value[4];
	*((uint8_t *)(data + 5)) = value[5];
}

static inline void get_ieee8021q_source(void *data, uint8_t value[6])
{
	value[0] = *((uint8_t *)(data + 6));
	value[1] = *((uint8_t *)(data + 7));
	value[2] = *((uint8_t *)(data + 8));
	value[3] = *((uint8_t *)(data + 9));
	value[4] = *((uint8_t *)(data + 10));
	value[5] = *((uint8_t *)(data + 11));
}

static inline void set_ieee8021q_source(void *data, uint8_t value[6])
{
	*((uint8_t *)(data + 6))  = value[0];
	*((uint8_t *)(data + 7))  = value[1];
	*((uint8_t *)(data + 8))  = value[2];
	*((uint8_t *)(data + 9))  = value[3];
	*((uint8_t *)(data + 10)) = value[4];
	*((uint8_t *)(data + 11)) = value[5];
}

/**
 * Accessor - IEEE1722/1722a
 */
DEF_AVTP_ACCESSER_UINT8(subtype, 0)
DEF_AVTP_ACCESSER_UINT8(sequence_num, 2)
DEF_AVTP_ACCESSER_UINT32(timestamp, 12)
DEF_AVTP_ACCESSER_UINT16(stream_data_length, 20)

static inline void get_avtp_stream_id(void *data, uint8_t value[8])
{
	value[0] = *((uint8_t *)(data + 4 + AVTP_OFFSET));
	value[1] = *((uint8_t *)(data + 5 + AVTP_OFFSET));
	value[2] = *((uint8_t *)(data + 6 + AVTP_OFFSET));
	value[3] = *((uint8_t *)(data + 7 + AVTP_OFFSET));
	value[4] = *((uint8_t *)(data + 8 + AVTP_OFFSET));
	value[5] = *((uint8_t *)(data + 9 + AVTP_OFFSET));
	value[6] = *((uint8_t *)(data + 10 + AVTP_OFFSET));
	value[7] = *((uint8_t *)(data + 11 + AVTP_OFFSET));
}

static inline void set_avtp_stream_id(void *data, uint8_t value[8])
{
	*((uint8_t *)(data + 4 + AVTP_OFFSET))  = value[0];
	*((uint8_t *)(data + 5 + AVTP_OFFSET))  = value[1];
	*((uint8_t *)(data + 6 + AVTP_OFFSET))  = value[2];
	*((uint8_t *)(data + 7 + AVTP_OFFSET))  = value[3];
	*((uint8_t *)(data + 8 + AVTP_OFFSET))  = value[4];
	*((uint8_t *)(data + 9 + AVTP_OFFSET))  = value[5];
	*((uint8_t *)(data + 10 + AVTP_OFFSET)) = value[6];
	*((uint8_t *)(data + 11 + AVTP_OFFSET)) = value[7];
}

/**
 * Template - IEEE1722/1722a
 */
extern void copy_avtp_stream_template(void *data);
extern void copy_avtp_cvf_experimental_template(void *data);

#endif /* __AVTP_H__ */
