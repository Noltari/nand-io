// SPDX-License-Identifier: MIT

#if !defined(_COMMON_H_)
#define _COMMON_H_

#include <stdint.h>

/* Bits */
#if !defined(BIT)
#define BIT(x) (1 << x)
#endif /* BIT */

/* Endian */
uint16_t bswap16(uint16_t value);
uint32_t bswap32(uint32_t value);

#if defined(__BIG_ENDIAN__)
    #define be16toh(x) (x)
    #define be32toh(x) (x)
    #define htobe16(x) (x)
    #define htobe32(x) (x)

    #define le16toh(x) bswap16(x)
    #define le32toh(x) bswap32(x)
    #define htole16(x) bswap16(x)
    #define htole32(x) bswap32(x)
#else
    #define be16toh(x) bswap16(x)
    #define be32toh(x) bswap32(x)
    #define htobe16(x) bswap16(x)
    #define htobe32(x) bswap32(x)

    #define le16toh(x) (x)
    #define le32toh(x) (x)
    #define htole16(x) (x)
    #define htole32(x) (x)
#endif /* __BIG_ENDIAN__ */

/* IO buffer */
#if !defined(IO_BUFFER_SIZE)
#define IO_BUFFER_SIZE 4096
#endif /* IO_BUFFER_SIZE */

/* MIN */
#if !defined(MIN)
#define MIN(a, b) ((a > b) ? b : a)
#endif /* MIN */

/* NOP */
#if !defined(NOP)
#define NOP() do { __asm__ __volatile__ ("nop"); } while (0)
#endif /* NOP */

#endif /* _COMMON_H_ */
