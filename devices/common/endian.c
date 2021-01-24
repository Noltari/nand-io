// SPDX-License-Identifier: MIT

#include "common.h"

uint16_t bswap16(uint16_t value)
{
	uint16_t swap;

	swap = (value & 0xFF00) >> 8;
	swap |= (value & 0x00FF) << 8;

	return swap;
}

uint32_t bswap32(uint32_t value)
{
	uint32_t swap;

	swap = (value & 0xFF000000) >> 24;
	swap |= (value & 0x00FF0000) >> 8;
	swap |= (value & 0x0000FF00) << 8;
	swap |= (value & 0x000000FF) << 24;

	return swap;
}
