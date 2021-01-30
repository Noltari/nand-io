# SPDX-License-Identifier: MIT
"""NAND IO interface."""

import sys

import serial

from .common import convert_size, ctypes_from_bytes
from .const import PAGE_RW_RETRIES, PROTOCOL_VERSION, SERIAL_DEF_SPEED, SERIAL_DEVICES
from .crc import CRC16_START, CRC32_START, crc16, crc32
from .logger import INFO, Logger
from .nand import Nand
from .protocol import (
    CMD_BOOTLOADER,
    CMD_NAND_ID_CONFIG,
    CMD_NAND_ID_READ,
    CMD_NAND_PAGE_READ,
    CMD_PING,
    CMD_RESTART,
    PKT_MAGIC,
    IOBootloaderRX,
    IOCrc16,
    IOCrc32,
    IONandIdRX,
    IOPacketHeader,
    IOPingRX,
    IORestartRX,
)
from .serial import SerialDevice


class NandIO:
    """NAND IO."""

    def __init__(
        self,
        serial_device,
        logger_level=INFO,
        logger_stream=sys.stdout,
        pull_up=False,
        serial_speed=SERIAL_DEF_SPEED,
    ):
        """Init NAND IO."""
        self.log = Logger(level=logger_level, stream=logger_stream)
        self.pull_up = pull_up
        self.serial_device = serial_device
        self.serial_speed = serial_speed
        self.serial = None
        self.nand = None

    def bootloader(self):
        """Enter device bootloader."""
        self.log.info("Entering device bootloader...")
        self.pkt_tx(CMD_BOOTLOADER, None)

        bootloader_rx = self.pkt_rx(CMD_BOOTLOADER, IOBootloaderRX)
        if bootloader_rx is None:
            return False

        if bootloader_rx.supported:
            self.log.info(" Success!\n")
        else:
            self.log.info(" Not supported!\n")

        return True

    def close(self):
        """Close serial device."""
        if self.serial:
            self.serial.close()

    def open(self):
        """Open serial device."""
        try:
            self.serial = SerialDevice(
                device=self.serial_device,
                speed=self.serial_speed,
            )
        except serial.SerialException:
            self.log.error("Error opening %s.\n", self.serial_device)
            return False

        return True

    def ping(self):
        """Ping device."""
        self.pkt_tx(CMD_PING, None)

        ping_rx = self.pkt_rx(CMD_PING, IOPingRX)
        if ping_rx is None:
            return False
        if ping_rx.version != PROTOCOL_VERSION:
            return False

        self.log.info("Device:\n")
        if ping_rx.device in SERIAL_DEVICES:
            self.log.info("\tID: %s\n", SERIAL_DEVICES[ping_rx.device])
        else:
            self.log.info("\tID: %x\n", ping_rx.device)
        self.log.info("\tVersion: %x\n", ping_rx.version)
        self.log.info("\tSerial speed: %u\n", ping_rx.serial_speed)
        self.log.info("\tMemory free: %s\n", convert_size(ping_rx.memory_free))

        return True

    def pkt_rx(self, cmd, _data, _debug=False):
        """Receive packet from serial."""
        _bytes = self.serial.read(IOPacketHeader)
        if _debug:
            self.log.info("pkt_rx: hdr=")
            print(_bytes)
        hdr = ctypes_from_bytes(IOPacketHeader, _bytes)

        if hdr.magic != PKT_MAGIC:
            return None
        if hdr.cmd != cmd:
            return None

        _crc_bytes = self.serial.read(IOCrc16)
        if _debug:
            self.log.info("pkt_rx: crc=")
            print(_crc_bytes)
        hdr_crc = ctypes_from_bytes(IOCrc16, _crc_bytes)

        calc_crc = crc16(CRC16_START, _bytes, len(_bytes))
        if calc_crc != hdr_crc.crc:
            self.log.error(
                "RX: header CRC error! (%04X vs %04X)\n", hdr_crc.crc, calc_crc
            )
            return None

        _bytes = self.serial.read(_data)
        if _debug:
            self.log.info("pkt_rx: len=%d data=", len(_bytes))
            print(_bytes)
        if isinstance(_data, (bytes, bytearray)):
            data = _bytes
        else:
            data = ctypes_from_bytes(_data, _bytes)
        _crc_bytes = self.serial.read(IOCrc32)
        if _debug:
            self.log.info("pkt_rx: crc=")
            print(_crc_bytes)
        data_crc = ctypes_from_bytes(IOCrc32, _crc_bytes)

        calc_crc = crc32(CRC32_START, _bytes, len(_bytes))
        if calc_crc != data_crc.crc:
            self.log.error(
                "RX: data CRC error! (%08X vs %08X)\n", data_crc.crc, calc_crc
            )
            return None

        return data

    def pkt_tx(self, cmd, data, _debug=False):
        """Send packet over serial."""
        if data:
            data_len = len(data)
        else:
            data_len = 0

        pkt_hdr = IOPacketHeader(
            magic=PKT_MAGIC,
            cmd=cmd,
            data_len=data_len,
        )
        hdr_bytes = bytearray(pkt_hdr)
        crc = crc16(CRC16_START, hdr_bytes, len(hdr_bytes))
        hdr_crc = IOCrc16(crc)
        self.serial.write(hdr_bytes)
        self.serial.write(bytearray(hdr_crc))
        if _debug:
            self.log.info("pkt_tx: hdr=")
            print(hdr_bytes)
            self.log.info("pkt_tx: crc=")
            print(bytearray(hdr_crc))

        if data:
            data_bytes = bytearray(data)
            crc = crc32(CRC32_START, data_bytes, len(data_bytes))
            data_crc = IOCrc32(crc)
            self.serial.write(data_bytes)
            self.serial.write(bytearray(data_crc))
            if _debug:
                self.log.info("pkt_rx: len=%d data=", len(data_bytes))
                print(data_bytes)
                self.log.info("pkt_tx: crc=")
                print(bytearray(data_crc))

        self.serial.flush()

    def read(self, file):
        """Read from device."""
        page_bytes = bytearray(self.nand.raw_page_size)
        page = 0
        retries = PAGE_RW_RETRIES

        out = open(file, "wb")
        while page < self.nand.pages:
            read_tx = self.nand.page_config_bytes(page)
            self.pkt_tx(CMD_NAND_PAGE_READ, read_tx)

            page_bytes = self.pkt_rx(CMD_NAND_PAGE_READ, page_bytes)
            if page_bytes is None:
                retries -= 1
                self.log.error(
                    "\nError reading page %d! (%d retries left)\n", page, retries
                )
                if retries == 0:
                    out.close()
                    return False
                continue
            retries = PAGE_RW_RETRIES

            out.write(bytearray(page_bytes))
            page += 1

            read_percent = int(round(page * 100 / self.nand.pages, 0))
            self.log.info(
                "Reading NAND %d%% (page=%d/%d)\r", read_percent, page, self.nand.pages
            )

        self.log.info("\n")
        out.close()

        return True

    def restart(self):
        """Restart device."""
        self.log.info("Restarting device...")
        self.pkt_tx(CMD_RESTART, None)

        restart_rx = self.pkt_rx(CMD_RESTART, IORestartRX)
        if restart_rx is None:
            return False

        if restart_rx.supported:
            self.log.info(" Success!\n")
        else:
            self.log.info(" Not supported!\n")
        return True

    def show_info(self):
        """Show device info."""
        self.pkt_tx(CMD_NAND_ID_READ, None)

        nand_id = self.pkt_rx(CMD_NAND_ID_READ, IONandIdRX)
        if nand_id is None:
            return False

        self.nand = Nand(self.log, self.pull_up)
        self.nand.identify(nand_id)
        self.pkt_tx(CMD_NAND_ID_CONFIG, self.nand.config_bytes())

        return True

    def write(self, file):
        """Write to device."""
        self.log.info("NAND: write file=%s\n", file)
