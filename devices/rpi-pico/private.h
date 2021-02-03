// SPDX-License-Identifier: MIT

#if !defined(_PRIVATE_H_)
#define _PRIVATE_H_

#if defined(DEBUG)
#include <stdio.h>
#endif

#define LED_PIN 25

uint32_t millis(void);

void usb_init(void);
void usb_process(void);

#endif /* _PRIVATE_H_ */
