# SPDX-License-Identifier: MIT
"""NAND IO serial device."""

import ctypes
import inspect

import serial

from .const import SERIAL_BUFFER_SIZE, SERIAL_DEF_SPEED, SERIAL_DEF_TIMEOUT


class SerialDevice:
    """Serial device."""

    def __init__(
        self,
        device,
        speed=SERIAL_DEF_SPEED,
        timeout=SERIAL_DEF_TIMEOUT,
    ):
        """Init serial device."""
        self.device = device
        self.speed = speed
        self.timeout = timeout

        self.buffer_size = SERIAL_BUFFER_SIZE
        self.buffer = bytearray()
        self.serial = serial.Serial(self.device, self.speed, timeout=self.timeout)

    def close(self):
        """Close serial device."""
        if self.serial:
            self.serial.close()
            return True
        return False

    def flush(self):
        """Flush serial device."""
        if len(self.buffer) > 0:
            self.serial.write(self.buffer)
            self.serial.flush()
            self.buffer = bytearray()
            return True
        return False

    def read(self, arg):
        """Read from serial device."""
        self.flush()
        _bytes = None
        if inspect.isclass(arg):
            _bytes = self.read(ctypes.sizeof(arg))
        elif isinstance(arg, (bytes, bytearray)):
            _bytes = self.serial.read(len(arg))
        else:
            _bytes = self.serial.read(arg)
        return _bytes

    def write(self, arg):
        """Write to serial device."""
        self.buffer += bytearray(arg)
        while len(self.buffer) > self.buffer_size:
            self.serial.write(self.buffer[: self.buffer_size])
            self.buffer_size = self.buffer_size[self.buffer_size :]
