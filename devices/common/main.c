// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "crc.h"
#include "device.h"
#include "nand.h"
#include "protocol.h"

typedef enum {
	CMD_OK = 0,
	CMD_UNKNOWN,
	CMD_ERROR_TRANSFER,
	CMD_ERROR_CRC,
	CMD_ERROR_NOT_SUPPORTED,
} cmd_res_t;

typedef enum {
	PKT_OK = 0,
	PKT_RX_SERIAL_EMPTY,
	PKT_RX_SERIAL_ERROR,
	PKT_RX_CRC_ERROR
} pkt_res_t;

nand_cfg_rx NAND;

pkt_res_t data_receive(pkt_hdr_t *pkt_hdr, void* data, uint32_t data_len)
{
	uint32_t rx_crc = 0;
	int ser_len = serial_read(data, data_len);
	int crc_len = serial_read(&rx_crc, DATA_CRC_LEN);

	if (ser_len == data_len && crc_len == DATA_CRC_LEN) {
		uint32_t calc_crc = crc32(CRC32_START, data, data_len);
		if (rx_crc != calc_crc)
			return PKT_RX_CRC_ERROR;
	}

	return PKT_OK;
}

pkt_res_t pkt_receive(pkt_hdr_t *pkt_hdr, void* data, uint32_t data_len)
{
	uint32_t hdr_len = sizeof(pkt_hdr_t);
	int ser_len = serial_read(pkt_hdr, hdr_len);
	int res;

	if (ser_len == hdr_len && le32toh(pkt_hdr->magic) == PKT_MAGIC) {
		uint16_t hdr_crc = crc16(CRC16_START, pkt_hdr, PKT_HDR_CRC_LEN);

		if (hdr_crc == le16toh(pkt_hdr->hdr_crc))
			res = PKT_OK;
		else
			res = PKT_RX_CRC_ERROR;
	} else if (ser_len == 0) {
		res = PKT_RX_SERIAL_EMPTY;
	} else {
		res = PKT_RX_SERIAL_ERROR;
	}

	if (data && res == PKT_OK)
		res = data_receive(pkt_hdr, data, data_len);

	return res;
}

pkt_res_t pkt_send(cmd_id_t cmd_id, const void *data, uint32_t data_len)
{
	pkt_hdr_t pkt_hdr = {
		.magic = htole32(PKT_MAGIC),
		.cmd = htole16(cmd_id),
		.data_len = htole32(data_len),
	};
	uint16_t hdr_crc = crc16(CRC16_START, &pkt_hdr, PKT_HDR_CRC_LEN);
	uint32_t data_crc = 0;

	if (data && data_len) {
		data_crc = crc32(CRC32_START, data, data_len);
		data_crc = htole32(data_crc);
	}

	pkt_hdr.hdr_crc = htole16(hdr_crc);
	serial_write(&pkt_hdr, sizeof(pkt_hdr_t));

	if (data && data_len) {
		serial_write(data, data_len);
		serial_write(&data_crc, DATA_CRC_LEN);
	}

	if (!data || (data && data_len))
		serial_flush_output();

	return PKT_OK;
}

int cmd_bootloader(pkt_hdr_t *pkt_hdr)
{
	bootloader_tx data = {
#if defined(BOOTLOADER_SUPPORT)
		.supported = 1,
#else
		.supported = 0,
#endif
	};

	pkt_send(CMD_BOOTLOADER, &data, sizeof(data));

#if defined(BOOTLOADER_SUPPORT)
	device_msleep(100);
	device_bootloader();
#endif

	return CMD_ERROR_NOT_SUPPORTED;
}

void cmd_error(cmd_res_t cmd_res)
{
	error_tx data = {
		.error = cmd_res,
	};

	pkt_send(CMD_ERROR, &data, sizeof(data));
}

int cmd_nand_id_read(pkt_hdr_t *rx_hdr)
{
	nand_id_tx data;
	nand_cfg_rx nc;
	pkt_hdr_t tx_hdr;
	pkt_res_t pkt_res;

	memset(&data, 0, sizeof(data));
	nand_read_id(&data);

	pkt_send(CMD_NAND_ID_READ, &data, sizeof(data));

	while (!serial_available())
		NOP();

	pkt_res = pkt_receive(&tx_hdr, &nc, sizeof(nc));
	if (pkt_res == PKT_OK && tx_hdr.cmd == CMD_NAND_ID_CONFIG) {
		NAND.raw_page_size = le32toh(nc.raw_page_size);
		NAND.read_delay_us = le32toh(nc.read_delay_us);
		NAND.pull_up = nc.pull_up;
	}

	return CMD_OK;
}

int cmd_nand_page_read(pkt_hdr_t *rx_hdr)
{
	nand_addr_rx page;
	uint8_t buffer[IO_BUFFER_SIZE];
	uint32_t len = 0;
	uint32_t crc = CRC32_START;

	memset(&page, 0, sizeof(page));
	data_receive(rx_hdr, &page, sizeof(page));

	pkt_send(CMD_NAND_PAGE_READ, NULL, NAND.raw_page_size);

	while (len < NAND.raw_page_size) {
		uint32_t cur_len = MIN(IO_BUFFER_SIZE, NAND.raw_page_size);

		nand_read_page(&page, buffer, cur_len, len == 0);
		crc = crc32(crc, buffer, cur_len);
		serial_write(buffer, cur_len);

		len += cur_len;
	}
	serial_write(&crc, DATA_CRC_LEN);

	return CMD_OK;
}

int cmd_ping(pkt_hdr_t *pkt_hdr)
{
	ping_tx data = {
		.device = device_id(),
		.version = htole16(PROTOCOL_VERSION),
		.serial_speed = htole32(serial_get_baud()),
		.memory_free = htole32(device_freeram()),
	};

	pkt_send(CMD_PING, &data, sizeof(data));

	return CMD_OK;
}

int cmd_restart(pkt_hdr_t *pkt_hdr)
{
	reboot_tx data = {
#if defined(RESTART_SUPPORT)
		.supported = 1,
#else
		.supported = 0,
#endif
	};

	pkt_send(CMD_RESTART, &data, sizeof(data));

#if defined(RESTART_SUPPORT)
	device_msleep(100);
	device_restart();
#endif

	return CMD_ERROR_NOT_SUPPORTED;
}

void cmd_process(pkt_hdr_t *pkt_hdr)
{
	cmd_res_t res;

	switch (le16toh(pkt_hdr->cmd)) {
		case CMD_BOOTLOADER:
			res = cmd_bootloader(pkt_hdr);
			break;
		case CMD_NAND_ID_READ:
			res = cmd_nand_id_read(pkt_hdr);
			device_release_ports();
			break;
		case CMD_NAND_PAGE_READ:
			res = cmd_nand_page_read(pkt_hdr);
			device_release_ports();
			break;
		case CMD_PING:
			res = cmd_ping(pkt_hdr);
			break;
		case CMD_RESTART:
			res = cmd_restart(pkt_hdr);
			break;
		default:
			res = CMD_UNKNOWN;
			break;
	}

	if (res != CMD_OK)
		cmd_error(res);
}

int main(void)
{
	device_init();

	while (1) {
		serial_flush_input();

		while (!serial_busy()) {
			if (serial_available()) {
				pkt_hdr_t pkt_hdr;
				pkt_res_t pkt_res = pkt_receive(&pkt_hdr, NULL, 0);

				if (pkt_res == PKT_OK)
					cmd_process(&pkt_hdr);
				else
					cmd_error(CMD_ERROR_TRANSFER);
			}
		}
	}

	return 0;
}
