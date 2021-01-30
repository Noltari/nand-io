# SPDX-License-Identifier: MIT

F_CPU := 8000000UL
MMCU := at90usb1286

CC := avr-gcc
OBJCOPY := avr-objcopy
SIZE := avr-size
RM := rm -f

CFLAGS := -std=gnu99 -Wall -Os -I. -I../include \
	-DF_CPU=$(F_CPU) -mmcu=$(MMCU) -fdata-sections -ffunction-sections \
	-DBOOTLOADER_SUPPORT -DRESTART_SUPPORT
EXTRA_CFLAGS :=

LDFLAGS := -Wl,--gc-sections
EXTRA_LDFLAGS :=

%.elf:
	$(CC) -o $@ $^ $(CFLAGS) $(EXTRA_CFLAGS) $(LDFLAGS) $(EXTRA_LDFLAGS)
	$(SIZE) $^ $@

%.hex: %.elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(EXTRA_CFLAGS)

teensy.elf: device.o millis.o nand.o serial.o \
	../common/crc.o ../common/endian.o ../common/main.o ../common/nand.o

all: teensy.hex

clean:
	$(RM) *.elf *.hex *.o ../common/*.o all

.DEFAULT_GOAL := all
