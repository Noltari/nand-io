# SPDX-License-Identifier: MIT
"""NAND device."""

from .common import convert_size
from .const import (
    NAND_DEVICES,
    NAND_PAGE_ADDR_3B,
    NAND_PAGE_ADDR_4B,
    NM_BLOCK_SIZE,
    NM_BLOCK_SIZE_BASE,
    NM_BLOCK_SIZE_MASK,
    NM_BLOCK_SIZE_SHIFT,
    NM_BUS_WIDTH,
    NM_BUS_WIDTH_BASE,
    NM_BUS_WIDTH_MASK,
    NM_BUS_WIDTH_SHIFT,
    NM_DEVICES,
    NM_NAME,
    NM_OOB_SIZE,
    NM_OOB_SIZE_BASE,
    NM_OOB_SIZE_MASK,
    NM_OOB_SIZE_SHIFT,
    NM_OOB_SIZE_SUB_PAGE,
    NM_PAGE_ADDR_TYPE,
    NM_PAGE_SIZE,
    NM_PAGE_SIZE_BASE,
    NM_PAGE_SIZE_MASK,
    NM_PAGE_SIZE_SHIFT,
    NM_PLANE_SIZE,
    NM_PLANE_SIZE_BASE,
    NM_PLANE_SIZE_MASK,
    NM_PLANE_SIZE_SHIFT,
    NM_PLANES,
    NM_PLANES_BASE,
    NM_PLANES_MASK,
    NM_PLANES_SHIFT,
    NM_READ_DELAY_US,
)
from .protocol import IONandAddressTX, IONandConfigRX


class Nand:
    """NAND device."""

    def __init__(
        self,
        log,
        pull_up=False,
    ):
        """Init NAND device."""
        self.log = log
        self.pull_up = pull_up

        self.block_pages = 0
        self.block_size = 0
        self.blocks = 0
        self.bus_width = 0
        self.dev_id = 0
        self.mf_id = 0
        self.oob_size = 0
        self.page_addr_type = 0
        self.page_size = 0
        self.pages = 0
        self.plane_size = 0
        self.planes = 0
        self.raw_block_size = 0
        self.raw_page_size = 0
        self.raw_size = 0
        self.read_delay_us = 0
        self.size = 0

    def config_bytes(self):
        """NAND Config in byte array format."""
        return bytearray(self.config_ctypes())

    def config_ctypes(self):
        """NAND Config in ctypes format."""
        return IONandConfigRX(
            raw_page_size=self.raw_page_size,
            read_delay_us=self.read_delay_us,
            pull_up=self.pull_up,
        )

    def identify(self, nand_id):
        """Attempt to idenfify NAND device."""
        nand_mf = None
        nand_dev = None
        if nand_id.mf_id in NAND_DEVICES:
            nand_mf = NAND_DEVICES[nand_id.mf_id]
            if nand_id.dev_id in nand_mf[NM_DEVICES]:
                nand_dev = nand_mf[NM_DEVICES][nand_id.dev_id]

        if nand_mf:
            mf_str = "%s (0x%02X)" % (nand_mf[NM_NAME], nand_id.mf_id)
        else:
            mf_str = "Unknown (0x%02X)" % nand_id.mf_id

        if nand_dev:
            dev_str = "%s (0x%02X)" % (nand_dev[NM_NAME], nand_id.dev_id)
        else:
            dev_str = "Unknown (0x%02X)" % nand_id.dev_id

        self.log.info("NAND Info:\n")
        self.log.info("\tManufacturer: %s\n", mf_str)
        self.log.info("\tDevice: %s\n", dev_str)
        self.log.info("\tChip data: 0x%02X\n", nand_id.chip_data)
        self.log.info("\tSize data: 0x%02X\n", nand_id.size_data)
        self.log.info("\tPlane data: 0x%02X\n", nand_id.plane_data)

        if (nand_mf is None) or (nand_dev is None):
            return False

        self.mf_id = nand_id.mf_id
        self.dev_id = nand_id.dev_id

        if NM_READ_DELAY_US in nand_dev:
            self.read_delay_us = nand_dev[NM_READ_DELAY_US]

        if NM_PAGE_ADDR_TYPE in nand_dev:
            self.page_addr_type = nand_dev[NM_PAGE_ADDR_TYPE]

        if NM_BUS_WIDTH in nand_dev:
            self.bus_width = nand_dev[NM_BUS_WIDTH]
        else:
            self.bus_width = nand_dev[NM_BUS_WIDTH_BASE] << (
                (nand_id.size_data >> nand_dev[NM_BUS_WIDTH_SHIFT])
                & nand_dev[NM_BUS_WIDTH_MASK]
            )

        if NM_PAGE_SIZE in nand_dev:
            self.page_size = nand_dev[NM_PAGE_SIZE]
        else:
            self.page_size = nand_dev[NM_PAGE_SIZE_BASE] << (
                (nand_id.size_data >> nand_dev[NM_PAGE_SIZE_SHIFT])
                & nand_dev[NM_PAGE_SIZE_MASK]
            )

        if NM_BLOCK_SIZE in nand_dev:
            self.block_size = nand_dev[NM_BLOCK_SIZE]
        else:
            self.block_size = nand_dev[NM_BLOCK_SIZE_BASE] << (
                (nand_id.size_data >> nand_dev[NM_BLOCK_SIZE_SHIFT])
                & nand_dev[NM_BLOCK_SIZE_MASK]
            )

        if NM_PLANES in nand_dev:
            self.planes = nand_dev[NM_PLANES]
        else:
            self.planes = nand_dev[NM_PLANES_BASE] << (
                (nand_id.plane_data >> nand_dev[NM_PLANES_SHIFT])
                & nand_dev[NM_PLANES_MASK]
            )

        if NM_PLANE_SIZE in nand_dev:
            self.plane_size = nand_dev[NM_PLANE_SIZE]
        else:
            self.plane_size = nand_dev[NM_PLANE_SIZE_BASE] << (
                (nand_id.plane_data >> nand_dev[NM_PLANE_SIZE_SHIFT])
                & nand_dev[NM_PLANE_SIZE_MASK]
            )

        if NM_OOB_SIZE in nand_dev:
            self.oob_size = nand_dev[NM_OOB_SIZE]
        else:
            self.oob_size = nand_dev[NM_OOB_SIZE_BASE] << (
                (nand_id.size_data >> nand_dev[NM_OOB_SIZE_SHIFT])
                & nand_dev[NM_OOB_SIZE_MASK]
            )
            if NM_OOB_SIZE_SUB_PAGE in nand_dev:
                self.oob_size *= int(self.page_size / nand_dev[NM_OOB_SIZE_SUB_PAGE])

        self.block_pages = int(self.block_size / self.page_size)
        self.blocks = int(self.planes * (self.plane_size / self.block_size))
        self.pages = self.block_pages * self.blocks
        self.raw_page_size = self.page_size + self.oob_size
        self.raw_block_size = int(self.block_pages * self.raw_page_size)
        self.raw_size = self.blocks * self.raw_block_size
        self.size = self.blocks * self.block_size

        self.log.info("\t---\n")
        self.log.info("\tBus width: %d\n", self.bus_width)
        self.log.info("\tSize: %s\n", convert_size(self.size))
        self.log.info("\tRaw size: %s\n", convert_size(self.raw_size))
        self.log.info("\tOOB size: %d\n", self.oob_size)
        self.log.info("\tPage size: %d\n", self.page_size)
        self.log.info("\tRaw Page size: %d\n", self.raw_page_size)
        self.log.info("\tBlock size: %s\n", convert_size(self.block_size))
        self.log.info("\tRaw Block size: %s\n", convert_size(self.raw_block_size))
        self.log.info("\tPlane size: %s\n", convert_size(self.plane_size))
        self.log.info("\tNumber of planes: %d\n", self.planes)
        self.log.info("\tNumber of blocks: %d\n", self.blocks)
        self.log.info("\tNumber of pages: %d\n", self.pages)
        self.log.info("\tPages per block: %d\n", self.block_pages)

        return True

    def page_config_bytes(self, page):
        """Page Config in byte array format."""
        return bytearray(self.page_config_ctypes(page))

    def page_config_ctypes(self, page):
        """Page Config in ctypes format."""
        page_config = IONandAddressTX()
        if self.page_addr_type == NAND_PAGE_ADDR_3B:
            page_config.addr[0] = 0
            page_config.addr[1] = page & 0xFF
            page_config.addr[2] = (page >> 8) & 0xFF
            page_config.addr_len = 3
        elif self.page_addr_type == NAND_PAGE_ADDR_4B:
            page_config.addr[0] = 0
            page_config.addr[1] = page & 0xFF
            page_config.addr[2] = (page >> 8) & 0xFF
            page_config.addr[3] = (page >> 16) & 0xFF
            page_config.addr_len = 4
        else:
            page_config.addr[0] = 0
            page_config.addr[1] = 0
            page_config.addr[2] = page & 0xFF
            page_config.addr[3] = (page >> 8) & 0xFF
            page_config.addr[4] = (page >> 16) & 0xFF
            page_config.addr_len = 5
        return page_config
