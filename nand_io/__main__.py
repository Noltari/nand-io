# SPDX-License-Identifier: MIT
"""NAND IO main."""

import argparse

from .common import auto_int
from .const import SERIAL_DEF_SPEED
from .interface import NandIO
from .logger import INFO


def main():
    """NAND IO."""
    parser = argparse.ArgumentParser(description="")

    parser.add_argument(
        "--bootloader",
        dest="bootloader",
        action="store_true",
        help="Force device bootloader",
    )

    parser.add_argument(
        "--read",
        dest="nand_read",
        action="store",
        type=str,
        help="NAND read",
    )

    parser.add_argument(
        "--restart",
        dest="restart",
        action="store_true",
        help="Force device restart",
    )

    parser.add_argument(
        "--serial-device",
        dest="serial_device",
        action="store",
        type=str,
        help="Serial device",
    )

    parser.add_argument(
        "--serial-speed",
        dest="serial_speed",
        action="store",
        type=auto_int,
        help="Serial speed",
    )

    parser.add_argument(
        "--show-info",
        dest="show_info",
        action="store_true",
        help="Show NAND info",
    )

    parser.add_argument(
        "--write",
        dest="nand_write",
        action="store",
        type=str,
        help="NAND write",
    )

    args = parser.parse_args()

    error = False
    if args.bootloader:
        if not args.serial_device:
            parser.print_help()
            error = True
    elif args.restart:
        if not args.serial_device:
            parser.print_help()
            error = True
    elif args.show_info:
        if not args.serial_device:
            parser.print_help()
            error = True
    elif args.nand_read:
        if not args.serial_device:
            parser.print_help()
            error = True
    elif args.nand_write:
        if not args.serial_device:
            parser.print_help()
            error = True
    else:
        parser.print_help()
        error = True

    if error:
        return

    if not args.serial_speed:
        args.serial_speed = SERIAL_DEF_SPEED

    nand = NandIO(
        serial_device=args.serial_device,
        serial_speed=args.serial_speed,
        logger_level=INFO,
    )
    if nand:
        if nand.open():
            if nand.ping():
                if args.bootloader:
                    nand.bootloader()
                elif args.restart:
                    nand.restart()
                elif args.show_info:
                    nand.show_info()
                elif args.nand_read:
                    nand.show_info()
                    nand.read(file=args.nand_read)
                elif args.nand_write:
                    nand.show_info()
                    nand.write(file=args.nand_write)
        nand.close()


main()
