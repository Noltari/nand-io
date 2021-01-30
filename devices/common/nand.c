// SPDX-License-Identifier: MIT

#include "common.h"
#include "device.h"
#include "nand.h"
#include "protocol.h"

extern nand_cfg_rx NAND;

void _nand_reset(void)
{
	nand_enable();

	nand_cmd(NC_RESET);

	nand_wait_rb();
}

int nand_read_id(nand_id_tx *nand_id)
{
	_nand_reset();

	nand_enable();

	nand_cmd(NC_READ_ID);

	nand_ale_high();
	nand_io_set(0);
	nand_ale_low();

	nand_io_in();
	nand_id->mf_id = nand_io_read();
	nand_id->dev_id = nand_io_read();
	nand_id->chip_data = nand_io_read();
	nand_id->size_data = nand_io_read();
	nand_id->plane_data = nand_io_read();

	return 0;
}

int nand_read_page(const nand_addr_rx *page, uint8_t *buffer, uint32_t len, int set)
{
	uint32_t offset;

	if (set) {
		nand_enable();

		nand_cmd(NC_READ1);

		nand_ale_high();
		for (offset = 0; offset < page->addr_len; offset++)
			nand_io_set(page->addr[offset]);
		nand_ale_low();

		if (NAND.read_delay_us)
			device_usleep(NAND.read_delay_us);
		else
			nand_cmd(NC_READ2);

		nand_io_in();
	}

	if (set)
		nand_wait_rb();

	for (offset = 0; offset < len; offset++)
		buffer[offset] = nand_io_read();

	return 0;
}
