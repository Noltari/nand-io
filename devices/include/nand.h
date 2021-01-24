// SPDX-License-Identifier: MIT

#if !defined(_NAND_H_)
#define _NAND_H_

#include "protocol.h"

#define NC_READ1	0x00
#define NC_PAGE_P2	0x10
#define NC_READ2	0x30
#define NC_ERASE1	0x60
#define NC_STATUS	0x70
#define NC_PAGE_P1	0x80
#define NC_READ_ID	0x90
#define NC_ERASE2	0xD0
#define NC_RESET	0xFF

int nand_read_id(nand_id_tx *nand_id);
int nand_read_page(const nand_page_cfg_rx *page, uint8_t *buffer, uint32_t len, int set);

#endif /* _NAND_H_ */
