// SPDX-License-Identifier: MIT

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "common.h"
#include "device.h"
#include "private.h"
#include "protocol.h"

#if !defined(WDFR)
#define WDFR 3
#endif /* WDFR */

void _device_release_all(void)
{
	EIMSK = 0;
	PCICR = 0;
	SPCR = 0;
	ACSR = 0;
	EECR = 0;
	ADCSRA = 0;
	TIMSK0 = 0;
	TIMSK1 = 0;
	TIMSK2 = 0;
	TIMSK3 = 0;
	UCSR1B = 0;
}

void device_bootloader(void)
{
	serial_end();

	cli();

	/* Watchdog */
	MCUSR &= ~BIT(WDFR);
	WDTCSR |= BIT(WDCE);
	WDTCSR = 0;
	_delay_ms(5);

	/* USB */
	UDCON = 1;
	USBCON = BIT(FRZCLK);
	UCSR1B = 0;
	_delay_ms(50);

	/* Ports */
	_device_release_all();
	device_release_ports();

	__asm volatile("jmp 0x1FC00");

	while (1)
		NOP();
}

uint32_t device_freeram(void)
{
	extern char *__brkval;
	char top;

	return &top - __brkval;
}

uint8_t device_id(void)
{
	return DEV_TEENSYPP2;
}

void device_init(void)
{
	/* CPU @ 8 MHz */
	CLKPR = 0x80;
	CLKPR = 1;

	/* Disable JTAG */
	MCUCR = BIT(JTD) | BIT(IVCE);
	MCUCR = BIT(JTD);

	device_release_ports();

	millis_init();

	serial_begin();
}

void device_msleep(uint32_t ms)
{
	while(ms--)
		_delay_ms(1);
}

void device_release_ports(void)
{
	DDRA = 0;
	DDRB = 0;
	DDRC = 0;
	DDRD = 0;
	DDRE = 0;
	DDRF = 0;
	PORTA = 0;
	PORTB = 0;
	PORTC = 0;
	PORTD = 0;
	PORTE = 0;
	PORTF = 0;
}

void device_restart(void)
{
	cli();

	_device_release_all();
	device_release_ports();
	_delay_ms(15);

	asm volatile("jmp 0");

	while (1)
		NOP();
}

void device_usleep(uint32_t us)
{
	while(us--)
		_delay_us(1);
}
