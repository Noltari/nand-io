// SPDX-License-Identifier: MIT

#include <pico/stdlib.h>
#include <stdint.h>
#include <tusb.h>

#include "common.h"
#include "device.h"
#include "private.h"

int serial_available(void)
{
	return (int) tud_cdc_available();
}

int serial_busy(void)
{
	return !tud_cdc_connected();
}

void serial_flush_input(void)
{
	tud_cdc_read_flush();
}

void serial_flush_output(void)
{
	tud_cdc_write_flush();
}

uint32_t serial_get_baud(void)
{
	cdc_line_coding_t cdc_lc;

	tud_cdc_get_line_coding(&cdc_lc);

	return cdc_lc.bit_rate;
}

size_t serial_read(void *ptr, size_t size)
{
	uint8_t *buffer = ptr;
	size_t off = 0;

	while (off < size) {
		int ch = tud_cdc_read_char();

		if (ch >= 0)
			buffer[off++] = (uint8_t) ch;
		else
			sleep_us(10);
	}

	return off;
}

size_t serial_write(const void *ptr, size_t size)
{
	const uint8_t *buffer = ptr;
	size_t off = 0;

	while (off < size) {
		int ch = tud_cdc_write_char(buffer[off]);

		if (ch > 0)
			off++;
		else
			sleep_us(10);
	}

	return off;
}

void usb_init(void)
{
	tusb_init();
}

void usb_process(void)
{
	while (1) {
		tud_task();

		gpio_put(LED_PIN, tud_cdc_connected());
	}
}
