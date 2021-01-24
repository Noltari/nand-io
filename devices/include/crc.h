// SPDX-License-Identifier: MIT

#if !defined(_CRC_H_)
#define _CRC_H_

#include <stdint.h>

#define CRC16_START 0xA281
uint16_t crc16(uint16_t crc, const void *buffer, uint32_t len);

#define CRC32_START 0xFFFFFFFF
uint32_t crc32(uint32_t crc, const void *buffer, uint32_t len);

#endif /* _CRC_H_ */
