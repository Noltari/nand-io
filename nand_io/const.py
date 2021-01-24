# SPDX-License-Identifier: MIT
"""NAND IO constants."""

NM_BLOCK_SIZE = "block-size"
NM_BLOCK_SIZE_BASE = "block-size-base"
NM_BLOCK_SIZE_MASK = "block-size-mask"
NM_BLOCK_SIZE_SHIFT = "block-size-shift"
NM_BUS_WIDTH = "bus-width"
NM_BUS_WIDTH_BASE = "bus-size-base"
NM_BUS_WIDTH_MASK = "bus-size-mask"
NM_BUS_WIDTH_SHIFT = "bus-size-shift"
NM_DEVICES = "devices"
NM_LAYOUT = "layout"
NM_NAME = "name"
NM_OOB_SIZE = "oob-size"
NM_OOB_SIZE_BASE = "oob-size-base"
NM_OOB_SIZE_MASK = "oob-size-mask"
NM_OOB_SIZE_SHIFT = "oob-size-shift"
NM_OOB_SIZE_SUB_PAGE = "oob-size-sub-page"
NM_PAGE_ADDR_TYPE = "page-address-type"
NM_PAGE_SIZE = "page-size"
NM_PAGE_SIZE_BASE = "page-size-base"
NM_PAGE_SIZE_MASK = "page-size-mask"
NM_PAGE_SIZE_SHIFT = "page-size-shift"
NM_PLANES = "planes"
NM_PLANES_BASE = "planes-base"
NM_PLANES_MASK = "planes-mask"
NM_PLANES_SHIFT = "planes-shift"
NM_PLANE_SIZE = "plane-size"
NM_PLANE_SIZE_BASE = "plane-size-base"
NM_PLANE_SIZE_MASK = "plane-size-mask"
NM_PLANE_SIZE_SHIFT = "plane-size-shift"
NM_READ_DELAY_US = "read-delay-us"

NAND_PAGE_ADDR_3B = 1
NAND_PAGE_ADDR_4B = 2

NAND_DEVICES = {
    0xAD: {
        NM_NAME: "Hynix",
        NM_DEVICES: {
            0x73: {
                NM_NAME: "HY27US08281A",
                NM_BLOCK_SIZE: 16384,
                NM_BUS_WIDTH: 8,
                NM_PAGE_ADDR_TYPE: NAND_PAGE_ADDR_3B,
                NM_OOB_SIZE: 16,
                NM_PAGE_SIZE: 512,
                NM_PLANES: 1,
                NM_PLANE_SIZE: 16 * 1024 * 1024,
                NM_READ_DELAY_US: 1,
            },
        },
    },
    0xC8: {
        NM_NAME: "ESMT",
        NM_DEVICES: {
            0xDA: {
                NM_NAME: "F59L2G81A",
                NM_BLOCK_SIZE_BASE: 64 * 1024,
                NM_BLOCK_SIZE_MASK: 0x03,
                NM_BLOCK_SIZE_SHIFT: 4,
                NM_BUS_WIDTH_BASE: 8,
                NM_BUS_WIDTH_MASK: 0x01,
                NM_BUS_WIDTH_SHIFT: 6,
                NM_OOB_SIZE_BASE: 8,
                NM_OOB_SIZE_MASK: 0x01,
                NM_OOB_SIZE_SHIFT: 2,
                NM_OOB_SIZE_SUB_PAGE: 512,
                NM_PAGE_SIZE_BASE: 1024,
                NM_PAGE_SIZE_MASK: 0x03,
                NM_PAGE_SIZE_SHIFT: 0,
                NM_PLANES_BASE: 1,
                NM_PLANES_MASK: 0x03,
                NM_PLANES_SHIFT: 2,
                NM_PLANE_SIZE_BASE: 8 * 1024 * 1024,
                NM_PLANE_SIZE_MASK: 0x07,
                NM_PLANE_SIZE_SHIFT: 4,
            },
        },
    },
    0xEC: {
        NM_NAME: "Samsung",
        NM_DEVICES: {
            0x79: {
                NM_NAME: "K9T1G08U0M",
                NM_BLOCK_SIZE: 16384,
                NM_BUS_WIDTH: 8,
                NM_OOB_SIZE: 16,
                NM_PAGE_ADDR_TYPE: NAND_PAGE_ADDR_4B,
                NM_PAGE_SIZE: 512,
                NM_PLANES: 4,
                NM_PLANE_SIZE: 32 * 1024 * 1024,
                NM_READ_DELAY_US: 1,
            },
        },
    },
}

PAGE_RW_RETRIES = 3

PROTOCOL_VERSION = 1

SERIAL_BUFFER_SIZE = 32768
SERIAL_DEF_SPEED = 9600
SERIAL_DEF_TIMEOUT = 1
SERIAL_DEVICES = {
    1: "Teensy++ 2.0",
}
