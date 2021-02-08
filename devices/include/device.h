// SPDX-License-Identifier: MIT

#if !defined(_DEVICE_H_)
#define _DEVICE_H_

#include <stddef.h>
#include <stdint.h>

void device_bootloader(void);
uint32_t device_freeram(void);
uint8_t device_id(void);
void device_init(void);
void device_msleep(uint32_t ms);
void device_restart(void);
void device_release_ports(void);
void device_usleep(uint32_t us);

int serial_available(void);
int serial_busy(void);
void serial_flush_input(void);
void serial_flush_output(void);
uint32_t serial_get_baud(void);
size_t serial_read(void *ptr, size_t size);
size_t serial_write(const void *ptr, size_t size);

void nand_ale_high(void);
void nand_ale_low(void);
void nand_cmd(uint8_t cmd);
void nand_disable(void);
void nand_enable(void);
void nand_io_in(void);
void nand_io_out(void);
uint8_t nand_io_read(void);
void nand_io_set(uint8_t data);
int nand_wait_rb(void);
void nand_we(void);

#endif /* _DEVICE_H_ */
