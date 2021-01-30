# SPDX-License-Identifier: MIT
"""NAND IO protocol."""

import ctypes

# Device
CMD_PING = 0x10
CMD_BOOTLOADER = 0x11
CMD_RESTART = 0x12
# NAND
CMD_NAND_ID_READ = 0x30
CMD_NAND_ID_CONFIG = 0x31
CMD_NAND_PAGE_READ = 0x32
CMD_NAND_PAGE_WRITE = 0x33
CMD_NAND_BLOCK_ERASE = 0x34
# Error
CMD_ERROR = 0xF0

# Command Results
CMD_OK = 0
CMD_UNKNOWN = 1
CMD_ERROR_TRANSFER = 2
CMD_ERROR_CRC = 3
CMD_ERROR_NOT_SUPPORTED = 4

# Protocol Magic
PKT_MAGIC = 0xDEADC0DE

# Page address size
PAGE_ADDR_SIZE = 5


class IOBootloaderRX(ctypes.LittleEndianStructure):
    """Enter device bootloader (response)."""

    _pack_ = 1
    _fields_ = [
        ("supported", ctypes.c_uint8),
    ]


class IOCrc16(ctypes.LittleEndianStructure):
    """CRC16."""

    _pack_ = 1
    _fields_ = [
        ("crc", ctypes.c_uint16),
    ]


class IOCrc32(ctypes.LittleEndianStructure):
    """CRC32."""

    _pack_ = 1
    _fields_ = [
        ("crc", ctypes.c_uint32),
    ]


class IOErrorRX(ctypes.LittleEndianStructure):
    """Error reported by device (response)."""

    _pack_ = 1
    _fields_ = [
        ("cmd_res", ctypes.c_uint8),
    ]


class IOPacketHeader(ctypes.LittleEndianStructure):
    """Packet header."""

    _pack_ = 1
    _fields_ = [
        ("magic", ctypes.c_uint32),
        ("cmd", ctypes.c_uint16),
        ("data_len", ctypes.c_uint32),
    ]


class IOPingRX(ctypes.LittleEndianStructure):
    """Ping device (response)."""

    _pack_ = 1
    _fields_ = [
        ("device", ctypes.c_uint8),
        ("version", ctypes.c_uint16),
        ("serial_speed", ctypes.c_uint32),
        ("memory_free", ctypes.c_uint32),
    ]


class IORestartRX(ctypes.LittleEndianStructure):
    """Restart (response)."""

    _pack_ = 1
    _fields_ = [
        ("supported", ctypes.c_uint8),
    ]


class IONandAddressTX(ctypes.LittleEndianStructure):
    """NAND configuration (request)."""

    _pack_ = 1
    _fields_ = [
        ("addr", ctypes.c_uint8 * PAGE_ADDR_SIZE),
        ("addr_len", ctypes.c_uint8),
    ]


class IONandConfigRX(ctypes.LittleEndianStructure):
    """NAND configuration (request)."""

    _pack_ = 1
    _fields_ = [
        ("raw_page_size", ctypes.c_uint32),
        ("read_delay_us", ctypes.c_uint32),
        ("pull_up", ctypes.c_uint8),
    ]


class IONandIdRX(ctypes.LittleEndianStructure):
    """Scan connected NAND (response)."""

    _pack_ = 1
    _fields_ = [
        ("mf_id", ctypes.c_uint8),
        ("dev_id", ctypes.c_uint8),
        ("chip_data", ctypes.c_uint8),
        ("size_data", ctypes.c_uint8),
        ("plane_data", ctypes.c_uint8),
    ]
