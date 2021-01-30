Teensy++ 2.0
============

Tools needed
------------

```
avr-libc binutils-avr gcc-avr
teensy-loader-cli
```

Teensy programming
------------------

```bash
teensy_loader_cli -v --mcu=at90usb1286 -w teensy.hex
```

Teensy-NAND connection
----------------------

__Warning:__ Teensy++ 2.0 VCC pin can't be used to power 3.3V NANDs unless it has been [converted to run at 3.3V with an external regulator](https://www.pjrc.com/teensy/3volt.html).

| Teensy Pin     | NAND Pin |
|:--------------:|:--------:|
| 3.3V           | VCC      |
| GND            | GND      |
| A0 A1 A2 A3 A4 | RE       |
| B0 B1 B2 B3 B4 | CLE      |
| C0 C1 C2 C3 C4 | WE       |
| D0 D1 D2 D3 D4 | ALE      |
| E6             | WP       |
| E7             | R/B      |
| F0             | I/O-0    |
| F1             | I/O-1    |
| F2             | I/O-2    |
| F3             | I/O-3    |
| F4             | I/O-4    |
| F5             | I/O-5    |
| F6             | I/O-6    |
| F7             | I/O-7    |
