// SPDX-License-Identifier: MIT

#include <hardware/gpio.h>
#include <pico/time.h>

#include "common.h"
#include "nand.h"
#include "private.h"

extern nand_cfg_rx NAND;

#define GPIO_IO_PINS 8
#define GPIO_IO_SHIFT 2
#define GPIO_IO_MASK (0xFF << GPIO_IO_SHIFT)
#define GPIO_RE 10
#define GPIO_WE 11
#define GPIO_ALE 12
#define GPIO_CLE 13
#define GPIO_WP 14
#define GPIO_RB 15
#define GPIO_CE 16

static inline void _gpio_pull_down_mask(uint32_t mask)
{
	uint8_t pin;

	for (pin = 0; pin < NUM_BANK0_GPIOS; pin++)
		if (mask & BIT(pin))
			gpio_pull_down(pin);
}

static inline void _gpio_pull_up_mask(uint32_t mask)
{
	uint8_t pin;

	for (pin = 0; pin < NUM_BANK0_GPIOS; pin++)
		if (mask & BIT(pin))
			gpio_pull_up(pin);
}

static inline void _nand_ale_high(void)
{
	gpio_put(GPIO_ALE, 1);
}

static inline void _nand_ale_low(void)
{
	gpio_put(GPIO_ALE, 0);
}

static inline void _nand_cle_high(void)
{
	gpio_put(GPIO_CLE, 1);
}

static inline void _nand_cle_low(void)
{
	gpio_put(GPIO_CLE, 0);
}

static inline void _nand_init(void)
{
	gpio_init_mask(GPIO_IO_MASK |
		GPIO_RE |
		GPIO_WE |
		GPIO_ALE |
		GPIO_CLE |
		GPIO_WP |
		GPIO_RB |
		GPIO_CE);

	gpio_set_dir_out_masked(GPIO_IO_MASK |
		GPIO_RE |
		GPIO_WE |
		GPIO_ALE |
		GPIO_CLE |
		GPIO_WP |
		GPIO_CE);
	gpio_set_dir_in_masked(GPIO_RB);

	gpio_clr_mask(GPIO_IO_MASK |
		GPIO_WE |
		GPIO_ALE |
		GPIO_CLE |
		GPIO_WP);
	gpio_set_mask(GPIO_RE |
		GPIO_CE);
}

static inline uint8_t _nand_io_get(void)
{
	return (uint8_t) ((gpio_get_all() & GPIO_IO_MASK) >> GPIO_IO_SHIFT);
}

static inline void _nand_io_in(void)
{
	gpio_set_dir_in_masked(GPIO_IO_MASK);
	if (NAND.pull_up)
		_gpio_pull_up_mask(GPIO_IO_MASK);
	else
		_gpio_pull_down_mask(GPIO_IO_MASK);
}

static inline void _nand_io_out(void)
{
	gpio_set_dir_out_masked(GPIO_IO_MASK);
}

static inline void _nand_io_set(uint8_t val)
{
	gpio_put_masked(GPIO_IO_MASK, val << GPIO_IO_SHIFT);
}

static inline void _nand_re_high(void)
{
	gpio_put(GPIO_RE, 1);
}

static inline void _nand_re_low(void)
{
	gpio_put(GPIO_RE, 0);
}

static inline void _nand_we(void)
{
	gpio_put(GPIO_WE, 0);
	gpio_put(GPIO_WE, 1);
}

void nand_ale_high(void)
{
	_nand_ale_high();
}

void nand_ale_low(void)
{
	_nand_ale_low();
}

void nand_cmd(uint8_t cmd)
{
	_nand_io_set(cmd);
	_nand_cle_high();
	_nand_we();
	_nand_cle_low();
}

void nand_disable(void)
{
	_nand_init();
}

void nand_enable(void)
{
	_nand_init();

	gpio_clr_mask(GPIO_CE);
}

void nand_io_in(void)
{
	_nand_io_in();
}

void nand_io_out(void)
{
	_nand_io_out();
}

uint8_t nand_io_read(void)
{
	uint8_t val;

	_nand_re_low();
	sleep_us(1);
	val = _nand_io_get();
	_nand_re_high();

	return val;
}

void nand_io_set(uint8_t data)
{
	_nand_io_set(data);
	_nand_we();
}

int nand_wait_rb(void)
{
	const uint32_t start = millis();

	while (millis() <= start + RB_TOUT_MS)
		if (gpio_get(GPIO_RB))
			return 1;

	return 0;
}

void nand_we(void)
{
	_nand_we();
}
