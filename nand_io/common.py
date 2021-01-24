# SPDX-License-Identifier: MIT
"""NAND IO Tools common."""

import ctypes
import math


def auto_int(int_arg):
    """Automatically format integer."""
    return int(int_arg, 0)


def convert_size(size_bytes):
    """Convert size to human readable."""
    if size_bytes is None or size_bytes == 0:
        return "0"
    size_name = ("", "K", "M", "G", "T", "P", "E", "Z", "Y")
    size_offset = int(math.floor(math.log(size_bytes, 1024)))
    size_pow = math.pow(1024, size_offset)
    size_value = round(size_bytes / size_pow, 2)
    if size_value.is_integer():
        size_value = int(size_value)
    return "%s%s" % (size_value, size_name[size_offset])


def ctypes_from_bytes(_class, _bytes):
    """Convert byte array to ctypes class."""
    return _class.from_buffer_copy(_bytes[: ctypes.sizeof(_class)])
