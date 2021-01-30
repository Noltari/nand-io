// SPDX-License-Identifier: MIT

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>
#include <stdint.h>

#include "common.h"

volatile uint32_t timer_ms; 

ISR(TIMER1_COMPA_vect)
{
	timer_ms++;
}

void millis_init(void)
{
	uint32_t ctc = ((F_CPU / 1000) / 8);

	TCCR1B |= BIT(WGM12) | BIT(CS11);

	OCR1AH = (ctc >> 8);
	OCR1AL = ctc;
 
	TIMSK1 |= BIT(OCIE1A);
}

uint32_t millis(void)
{
	uint32_t _ms;
 
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		_ms = timer_ms;
	}

	return _ms;
}
