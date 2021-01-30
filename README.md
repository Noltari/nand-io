NAND IO
=======

Python tool for NAND IO over a serial device.

Disclaimer
----------

This software is provided without warranty, according to the MIT License, and should therefore not be used where it may endanger life, financial stakes, or cause discomfort and inconvenience to others.

Supported Devices
-----------------

- Teensy++ 2.0

Usage
-----

```bash
python3 -m nand_io --serial-device /dev/ttyACM0
```

Serial Communication Protocol
-----------------------------

- Packet header:
  - u32: magic (0xDEADC0DE)
  - u16: command
  - u32: data length
  - u16: header CRC16
- Data (optional):
  - u8: data[data length]
  - u32: data CRC32

Exchanged data should be in little endian.
