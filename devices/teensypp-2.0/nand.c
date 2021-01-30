// SPDX-License-Identifier: MIT

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "common.h"
#include "nand.h"
#include "private.h"

#define DDR_RE		DDRA
#define PIN_RE		PINA
#define PORT_RE		PORTA

#define DDR_CLE		DDRB
#define PIN_CLE		PINB
#define PORT_CLE	PORTB

#define DDR_WE		DDRC
#define PIN_WE		PINC
#define PORT_WE		PORTC

#define DDR_ALE		DDRD
#define PIN_ALE		PIND
#define PORT_ALE	PORTD

#define DDR_RB_WP	DDRE
#define PIN_RB_WP	PINE
#define PORT_RB_WP	PORTE
#define PIN_WP		BIT(6)
#define PIN_RB		BIT(7)

#define DDR_IO		DDRF
#define PIN_IO		PINF
#define PORT_IO		PORTF

#define RB_TOUT_MS	3000

extern nand_cfg_rx NAND;

void nand_ale_high(void)
{
	PORT_ALE = 0xFF;
}

void nand_ale_low(void)
{
	PORT_ALE = 0;
}

void nand_we(void)
{
	PORT_WE = 0;
	PORT_WE = 0xFF;
}

void nand_cmd(uint8_t cmd)
{
	PORT_IO = cmd;
	PORT_CLE = 0xFF;
	nand_we();
	PORT_CLE = 0;
}

void nand_io_in(void)
{
	DDR_IO = 0;
	if (NAND.pull_up)
		PORT_IO = 0xFF;
	else
		PORT_IO = 0;
}

void nand_io_out(void)
{
	DDR_IO = 0xFF;
}

uint8_t nand_io_read(void)
{
	uint8_t data;

	PORT_RE = 0;
	_delay_us(0.2);
	data = PIN_IO;
	PORT_RE = 0xFF;

	return data;
}

void nand_io_set(uint8_t data)
{
	PORT_IO = data;
	nand_we();
}

void nand_enable(void)
{
	DDR_RB_WP = 0xFF;
	DDR_RB_WP = (uint8_t) ~PIN_RB;
	PORT_RB_WP = PIN_WP | PIN_RB;

	DDR_WE = 0xFF;
	PORT_WE = 0xFF;

	DDR_RE = 0xFF;
	PORT_RE = 0xFF;

	DDR_ALE = 0xFF;
	PORT_ALE = 0;

	DDR_CLE = 0xFF;
	PORT_CLE = 0;

	nand_io_out();
}

int nand_wait_rb(void)
{
	const uint32_t start = millis();

	while (millis() <= start + RB_TOUT_MS)
		if (PIN_RB_WP & PIN_RB)
			return 1;

	return 0;
}
