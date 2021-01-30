// SPDX-License-Identifier: MIT

#if !defined(_PROTOCOL_H_)
#define _PROTOCOL_H_

#define PACKED __attribute__((packed))

#define PROTOCOL_VERSION 1

typedef enum {
	DEV_UNKNOWN = 0,
	DEV_TEENSYPP2 = 1,
} dev_id_t;

typedef enum {
	/* Device */
	CMD_PING = 0x10,
	CMD_BOOTLOADER = 0x11,
	CMD_RESTART = 0x12,
	/* NAND */
	CMD_NAND_ID_READ = 0x30,
	CMD_NAND_ID_CONFIG = 0x31,
	CMD_NAND_PAGE_READ = 0x32,
	CMD_NAND_PAGE_WRITE = 0x33,
	CMD_NAND_BLOCK_ERASE = 0x34,
	/* Error */
	CMD_ERROR = 0xF0,
} cmd_id_t;

#define PKT_MAGIC 0xDEADC0DE
typedef struct {
	uint32_t magic;
	uint16_t cmd;
	uint32_t data_len;
	uint16_t hdr_crc;
} PACKED pkt_hdr_t;
#define PKT_HDR_CRC_LEN (sizeof(pkt_hdr_t) - sizeof(uint16_t))

typedef struct {
	uint8_t supported;
} PACKED bootloader_tx;

typedef struct {
	uint8_t error;
} PACKED error_tx;

#define NAND_ADDR_SIZE 5
typedef struct {
	uint8_t addr[NAND_ADDR_SIZE];
	uint8_t addr_len;
} PACKED nand_addr_rx;

typedef struct {
	uint32_t raw_page_size;
	uint32_t read_delay_us;
	uint8_t pull_up;
} PACKED nand_cfg_rx;

typedef struct {
	uint8_t mf_id;
	uint8_t dev_id;
	uint8_t chip_data;
	uint8_t size_data;
	uint8_t plane_data;
} PACKED nand_id_tx;

typedef struct {
	uint8_t device;
	uint16_t version;
	uint32_t serial_speed;
	uint32_t memory_free;
} PACKED ping_tx;

typedef struct {
	uint8_t supported;
} PACKED reboot_tx;

#define DATA_CRC_LEN (sizeof(uint32_t))

#endif /* _PROTOCOL_H_ */
