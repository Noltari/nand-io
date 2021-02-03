# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.13)

include(pico-sdk/pico_sdk_init.cmake)

project(pico_nand_io)

pico_sdk_init()
  
add_executable(nand_io
	device.c
	nand.c
	serial.c
	usb-descriptors.c
	../common/crc.c
	../common/endian.c
	../common/main.c
	../common/nand.c)

target_compile_definitions(nand_io PRIVATE
	DEBUG
	BOOTLOADER_SUPPORT
	RESTART_SUPPORT)

target_include_directories(nand_io PUBLIC
	pico-sdk/lib/tinyusb/src
	./
	../include)

target_link_libraries(nand_io
	pico_multicore
	pico_stdlib
	tinyusb_device)

pico_add_extra_outputs(nand_io)
