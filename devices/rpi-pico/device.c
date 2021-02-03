// SPDX-License-Identifier: MIT

#include <hardware/structs/sio.h>
#include <hardware/watchdog.h>
#include <pico/bootrom.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/time.h>

#include "common.h"
#include "device.h"
#include "private.h"
#include "protocol.h"

void device_bootloader(void)
{
	device_release_ports();

	reset_usb_boot(0, 0);

	while (1)
		NOP();
}

uint32_t device_freeram(void)
{
	uint8_t dummy;
	uint32_t addr = (uint32_t) &dummy;

	return addr - SRAM_BASE;
}

uint8_t device_id(void)
{
	return DEV_RPI_PICO;
}

void device_init(void)
{
	device_release_ports();

#if defined(DEBUG)
	stdio_init_all();
#endif

	usb_init();

	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	multicore_launch_core1(usb_process);
}

void device_msleep(uint32_t ms)
{
	return sleep_ms(ms);
}

void device_release_ports(void)
{
	nand_disable();
}

void device_restart(void)
{
	device_release_ports();

	watchdog_reboot(0, SRAM_END, 0);

	while (1)
		NOP();
}

void device_usleep(uint32_t us)
{
	return sleep_us(us);
}

uint32_t millis(void)
{
	return to_ms_since_boot(get_absolute_time());
}
