// SPDX-License-Identifier: MIT

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdint.h>

#include "common.h"
#include "device.h"
#include "private.h"

#define SERIAL_TIMEOUT_MS		1000
#define TRANSMIT_FLUSH_TIMEOUT	3
#define TRANSMIT_TIMEOUT		15

#define USB_CONFIG_MS	200
#define USB_TIMEOUT_MS	2500
#define USB_SUSPEND_MS	250

#define MAX_ENDPOINT	6

#define LSB(n) (n & 0xFF)
#define MSB(n) ((n >> 8) & 0xFF)

#define EP_TYPE_CONTROL			0x00
#define EP_TYPE_BULK_IN			0x81
#define EP_TYPE_BULK_OUT		0x80
#define EP_TYPE_INTERRUPT_IN	0xC1
#define EP_SINGLE_BUFFER		0x02
#define EP_DOUBLE_BUFFER		0x06
#define EP_SIZE(s) \
	((s) == 64 ? 0x30 : ((s) == 32 ? 0x20 : ((s) == 16 ? 0x10 : 0x00)))

#define GET_STATUS			0
#define CLEAR_FEATURE		1
#define SET_FEATURE			3
#define SET_ADDRESS			5
#define GET_DESCRIPTOR		6
#define GET_CONFIGURATION	8
#define SET_CONFIGURATION	9

#define CDC_SET_LINE_CODING			0x20
#define CDC_GET_LINE_CODING			0x21
#define CDC_SET_CONTROL_LINE_STATE	0x22
#define CDC_SEND_BREAK				0x23

#define pgm_read_byte_postinc(val, addr) \
	asm ("lpm %0, Z+\n" : "=r" (val), "+z" (addr) : )
#define pgm_read_word_postinc(val, addr) \
	asm ("lpm %A0, Z+\n\tlpm %B0, Z+\n" : "=r" (val), "+z" (addr) : )

#define read_word_lsbfirst(val, reg) \
	asm volatile( \
		"lds %A0, %1\n\tlds %B0, %1\n" \
		: "=r" (val) : "M" ((int)(&reg)) )

#define USBSTATE __attribute__ ((section (".noinit")))

#define STR_MANUFACTURER	L"Teensyduino"
#define STR_PRODUCT			L"USB Serial"
#define STR_SERIAL_NUMBER	L"12345"

#define VENDOR_ID	0x16C0
#define PRODUCT_ID	0x0483

#define ENDPOINT0_SIZE		32
#define CDC_ACM_ENDPOINT	2
#define CDC_ACM_SIZE		8
#define CDC_ACM_BUFFER		EP_SINGLE_BUFFER
#define CDC_RX_ENDPOINT		3
#define CDC_RX_BUFFER		EP_DOUBLE_BUFFER
#define CDC_TX_ENDPOINT		4
#define CDC_TX_BUFFER		EP_DOUBLE_BUFFER

#define CDC_RX_SIZE			64
#define CDC_TX_SIZE			64

#define ASM_COPY1(src, dest, tmp) \
	"ld " tmp ", " src "\n\t" "st " dest ", " tmp "\n\t"
#define ASM_COPY2(src, dest, tmp) \
	ASM_COPY1(src, dest, tmp) ASM_COPY1(src, dest, tmp)
#define ASM_COPY4(src, dest, tmp) \
	ASM_COPY2(src, dest, tmp) ASM_COPY2(src, dest, tmp)
#define ASM_COPY8(src, dest, tmp) \
	ASM_COPY4(src, dest, tmp) ASM_COPY4(src, dest, tmp)

static const uint8_t PROGMEM endpoint_config_table[] = {
	0,
	1, EP_TYPE_INTERRUPT_IN, EP_SIZE(CDC_ACM_SIZE) | CDC_ACM_BUFFER,
	1, EP_TYPE_BULK_OUT, EP_SIZE(CDC_RX_SIZE) | CDC_RX_BUFFER,
	1, EP_TYPE_BULK_IN, EP_SIZE(CDC_TX_SIZE) | CDC_TX_BUFFER
};

static const uint8_t PROGMEM device_descriptor[] = {
	18,									// bLength
	1,									// bDescriptorType
	0x00, 0x02,							// bcdUSB
	2,									// bDeviceClass
	0,									// bDeviceSubClass
	0,									// bDeviceProtocol
	ENDPOINT0_SIZE,						// bMaxPacketSize0
	LSB(VENDOR_ID), MSB(VENDOR_ID),		// idVendor
	LSB(PRODUCT_ID), MSB(PRODUCT_ID),	// idProduct
	0x72, 0x02,
	1,									// iManufacturer
	2,									// iProduct
	3,									// iSerialNumber
	1									// bNumConfigurations
};

#define CONFIG1_DESC_SIZE (9+9+5+5+4+5+7+9+7+7)
static const uint8_t PROGMEM config1_descriptor[CONFIG1_DESC_SIZE] = {
	/* Configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10 */
	9,							// bLength;
	2,							// bDescriptorType;
	LSB(CONFIG1_DESC_SIZE),		// wTotalLength
	MSB(CONFIG1_DESC_SIZE),
	2,							// bNumInterfaces
	1,							// bConfigurationValue
	0,							// iConfiguration
	0xC0,						// bmAttributes
	50,							// bMaxPower
	/* Interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12 */
	9,		// bLength
	4,		// bDescriptorType
	0,		// bInterfaceNumber
	0,		// bAlternateSetting
	1,		// bNumEndpoints
	0x02,	// bInterfaceClass
	0x02,	// bInterfaceSubClass
	0x01,	// bInterfaceProtocol
	0,		// iInterface
	/* CDC Header Functional Descriptor, CDC Spec 5.2.3.1, Table 26 */
	5,			// bFunctionLength
	0x24,		// bDescriptorType
	0x00,		// bDescriptorSubtype
	0x10, 0x01,	// bcdCDC
	/* Call Management Functional Descriptor, CDC Spec 5.2.3.2, Table 27 */
	5,		// bFunctionLength
	0x24,	// bDescriptorType
	0x01,	// bDescriptorSubtype
	0x00,	// bmCapabilities
	1,		// bDataInterface
	/* Abstract Control Management Functional Descriptor, CDC Spec 5.2.3.3,
	   Table 28 */
	4,		// bFunctionLength
	0x24,	// bDescriptorType
	0x02,	// bDescriptorSubtype
	0x06,	// bmCapabilities
	/* Union Functional Descriptor, CDC Spec 5.2.3.8, Table 33 */
	5,		// bFunctionLength
	0x24,	// bDescriptorType
	0x06,	// bDescriptorSubtype
	0,		// bMasterInterface
	1,		// bSlaveInterface0
	/* Endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13 */
	7,							// bLength
	5,							// bDescriptorType
	CDC_ACM_ENDPOINT | 0x80,	// bEndpointAddress
	0x03,						// bmAttributes
	CDC_ACM_SIZE, 0,			// wMaxPacketSize
	64,							// bInterval
	/* Interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12 */
	9,		// bLength
	4,		// bDescriptorType
	1,		// bInterfaceNumber
	0,		// bAlternateSetting
	2,		// bNumEndpoints
	0x0A,	// bInterfaceClass
	0x00,	// bInterfaceSubClass
	0x00,	// bInterfaceProtocol
	0,		// iInterface
	/* Endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13 */
	7,					// bLength
	5,					// bDescriptorType
	CDC_RX_ENDPOINT,	// bEndpointAddress
	0x02,				// bmAttributes
	CDC_RX_SIZE, 0,		// wMaxPacketSize
	0,					// bInterval
	/* Endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13 */
	7,						// bLength
	5,						// bDescriptorType
	CDC_TX_ENDPOINT | 0x80,	// bEndpointAddress
	0x02,					// bmAttributes
	CDC_TX_SIZE, 0,			// wMaxPacketSize
	0						// bInterval
};

struct usb_string_descriptor_struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	int16_t wString[];
};

static const struct usb_string_descriptor_struct PROGMEM string0 = {
	4,
	3,
	{0x0409}
};

static const struct usb_string_descriptor_struct PROGMEM string1 = {
	sizeof(STR_MANUFACTURER),
	3,
	STR_MANUFACTURER
};

static const struct usb_string_descriptor_struct PROGMEM string2 = {
	sizeof(STR_PRODUCT),
	3,
	STR_PRODUCT
};

static const struct usb_string_descriptor_struct PROGMEM string3 = {
	sizeof(STR_SERIAL_NUMBER),
	3,
	STR_SERIAL_NUMBER
};

static const struct descriptor_list_struct {
	uint16_t wValue;
	uint16_t wIndex;
	const uint8_t *addr;
	uint8_t length;
} PROGMEM descriptor_list[] = {
	{0x0100, 0x0000, device_descriptor, sizeof(device_descriptor)},
	{0x0200, 0x0000, config1_descriptor, sizeof(config1_descriptor)},
	{0x0300, 0x0000, (const uint8_t *) &string0, 4},
	{0x0301, 0x0409, (const uint8_t *) &string1, sizeof(STR_MANUFACTURER)},
	{0x0302, 0x0409, (const uint8_t *) &string2, sizeof(STR_PRODUCT)},
	{0x0303, 0x0409, (const uint8_t *) &string3, sizeof(STR_SERIAL_NUMBER)}
};
#define NUM_DESC_LIST (sizeof(descriptor_list) / sizeof(struct descriptor_list_struct))

volatile uint8_t usb_configuration USBSTATE;
volatile uint8_t usb_suspended USBSTATE;

volatile uint8_t transmit_flush_timer = 0;
volatile uint8_t reboot_timer = 0;
uint8_t transmit_previous_timeout = 0;

volatile uint8_t cdc_line_coding[7] = {
	0x00, 0xE1, 0x00, 0x00, 0x00, 0x00, 0x08
};
volatile uint8_t cdc_line_rtsdtr USBSTATE;

int16_t peek_buf;

ISR(USB_GEN_vect)
{
	uint8_t intbits;
	uint8_t t;

	intbits = UDINT;
	UDINT = 0;
	if (intbits & BIT(EORSTI)) {
		UENUM = 0;
		UECONX = 1;
		UECFG0X = EP_TYPE_CONTROL;
		UECFG1X = EP_SIZE(ENDPOINT0_SIZE) | EP_SINGLE_BUFFER;
		UEIENX = BIT(RXSTPE);
		usb_configuration = 0;
		cdc_line_rtsdtr = 0;
	}

	if (intbits & BIT(SOFI)) {
		if (usb_configuration) {
			t = transmit_flush_timer;
			if (t) {
				transmit_flush_timer = --t;
				if (!t) {
					UENUM = CDC_TX_ENDPOINT;
					UEINTX = 0x3A;
				}
			}

			t = reboot_timer;
			if (t) {
				reboot_timer = --t;
				if (!t)
					device_bootloader();
			}
		}
	}

	if (intbits & BIT(SUSPI)) {
		UDIEN = BIT(WAKEUPE);

		usb_configuration = 0;
		usb_suspended = 1;

		USBCON = BIT(USBE) | BIT(FRZCLK);
		PLLCSR = 0;
	}

	if (usb_suspended && (intbits & BIT(WAKEUPI))) {
		PLLCSR = 0x16;
		while (!(PLLCSR & BIT(PLOCK)))
			NOP();

		USBCON = BIT(USBE) | BIT(OTGPADE);
		UDIEN = BIT(EORSTE) | BIT(SOFE) | BIT(SUSPE);

		usb_suspended = 0;
	}
}

static inline void usb_wait_in_ready(void)
{
	while (!(UEINTX & BIT(TXINI)))
		NOP();
}

static inline void usb_send_in(void)
{
	UEINTX = ~BIT(TXINI);
}

static inline void usb_wait_receive_out(void)
{
	while (!(UEINTX & BIT(RXOUTI)))
		NOP();
}

static inline void usb_ack_out(void)
{
	UEINTX = ~BIT(RXOUTI);
}

ISR(USB_COM_vect)
{
	uint8_t intbits;
	const uint8_t *list;
	const uint8_t *cfg;
	uint8_t i, n, len, en;
	volatile uint8_t *p;
	uint8_t bmRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
	uint16_t desc_val;
	uint32_t baud;
	const uint8_t *desc_addr;
	uint8_t	desc_length;

	UENUM = 0;
	intbits = UEINTX;
	if (intbits & BIT(RXSTPI)) {
		bmRequestType = UEDATX;
		bRequest = UEDATX;
		read_word_lsbfirst(wValue, UEDATX);
		read_word_lsbfirst(wIndex, UEDATX);
		read_word_lsbfirst(wLength, UEDATX);
		UEINTX = ~(BIT(RXSTPI) | BIT(RXOUTI) | BIT(TXINI));

		if (bRequest == GET_DESCRIPTOR) {
			list = (const uint8_t *)descriptor_list;

			for (i = 0; ; i++) {
				if (i >= NUM_DESC_LIST) {
					UECONX = BIT(STALLRQ) | BIT(EPEN);
					return;
				}

				pgm_read_word_postinc(desc_val, list);
				if (desc_val != wValue) {
					list += sizeof(struct descriptor_list_struct) - 2;
					continue;
				}

				pgm_read_word_postinc(desc_val, list);
				if (desc_val != wIndex) {
					list += sizeof(struct descriptor_list_struct) - 4;
					continue;
				}

				pgm_read_word_postinc(desc_addr, list);
				desc_length = pgm_read_byte(list);
				break;
			}

			len = (wLength <= 0xFF) ? wLength : 0xFF;
			if (len > desc_length)
				len = desc_length;

			list = desc_addr;
			do {
				do {
					i = UEINTX;
				} while (!(i & (BIT(TXINI) | BIT(RXOUTI))));

				if (i & BIT(RXOUTI))
					return;

				n = len < ENDPOINT0_SIZE ? len : ENDPOINT0_SIZE;
				for (i = n; i; i--)
					pgm_read_byte_postinc(UEDATX, list);

				len -= n;
				usb_send_in();
			} while (len || n == ENDPOINT0_SIZE);

			return;
		}

		if (bRequest == SET_ADDRESS) {
			usb_send_in();
			usb_wait_in_ready();
			UDADDR = wValue | BIT(ADDEN);
			return;
		}

		if (bRequest == SET_CONFIGURATION && bmRequestType == 0) {
			usb_configuration = wValue;
			cdc_line_rtsdtr = 0;
			transmit_flush_timer = 0;
			usb_send_in();

			cfg = endpoint_config_table;
			for (i = 1; i < 5; i++) {
				UENUM = i;
				pgm_read_byte_postinc(en, cfg);
				UECONX = en;

				if (en) {
					pgm_read_byte_postinc(UECFG0X, cfg);
					pgm_read_byte_postinc(UECFG1X, cfg);
				}
			}

			UERST = 0x1E;
			UERST = 0;

			return;
		}

		if (bRequest == GET_CONFIGURATION && bmRequestType == 0x80) {
			usb_wait_in_ready();
			UEDATX = usb_configuration;
			usb_send_in();
			return;
		}

		if (bRequest == CDC_GET_LINE_CODING && bmRequestType == 0xA1) {
			usb_wait_in_ready();

			p = cdc_line_coding;
			for (i = 0; i < 7; i++)
				UEDATX = *p++;

			usb_send_in();
			return;
		}

		if (bRequest == CDC_SET_LINE_CODING && bmRequestType == 0x21) {
			usb_wait_receive_out();

			p = cdc_line_coding;
			for (i = 0; i < 7; i++)
				*p++ = UEDATX;

			usb_ack_out();
			usb_send_in();

			baud = serial_get_baud();
			if (baud == 134UL) {
				reboot_timer = 15;
			} else if (baud == 150UL) {
				UENUM = CDC_TX_ENDPOINT;

				while (UESTA0X & 0x03) {
					UEINTX = 0xFF;
					while (UEINTX & 0x04)
						NOP();
				}

				device_restart();
			}

			return;
		}

		if (bRequest == CDC_SET_CONTROL_LINE_STATE && bmRequestType == 0x21) {
			cdc_line_rtsdtr = wValue;
			usb_wait_in_ready();
			usb_send_in();

			return;
		}

		if (bRequest == CDC_SEND_BREAK && bmRequestType == 0x21) {
			usb_wait_in_ready();
			usb_send_in();

			return;
		}

		if (bRequest == GET_STATUS) {
			usb_wait_in_ready();

			i = 0;
			if (bmRequestType == 0x82) {
				UENUM = wIndex;
				if (UECONX & BIT(STALLRQ))
					i = 1;
				UENUM = 0;
			}

			UEDATX = i;
			UEDATX = 0;
			usb_send_in();

			return;
		}

		if ((bRequest == CLEAR_FEATURE || bRequest == SET_FEATURE) &&
			bmRequestType == 0x02 && wValue == 0) {
			i = wIndex & 0x7F;
			if (i >= 1 && i <= MAX_ENDPOINT) {
				usb_send_in();
				UENUM = i;

				if (bRequest == SET_FEATURE) {
					UECONX = BIT(STALLRQ) | BIT(EPEN);
				} else {
					UECONX = BIT(STALLRQC) | BIT(RSTDT) | BIT(EPEN);
					UERST = BIT(i);
					UERST = 0;
				}

				return;
			}
		}
	}

	UECONX = BIT(STALLRQ) | BIT(EPEN);
}

void usb_init(void)
{
	uint8_t usb_con;

	peek_buf = -1;

	usb_con = USBCON;
	if ((usb_con & BIT(USBE)) && !(usb_con & BIT(FRZCLK)))
		return;

	UHWCON = 0x81;
	USBCON = BIT(USBE) | BIT(FRZCLK);
	PLLCSR = 0x16;
	while (!(PLLCSR & BIT(PLOCK)))
		NOP();
	USBCON = BIT(USBE) | BIT(OTGPADE);

	UDCON = 0;
	usb_configuration = 0;
	usb_suspended = 0;
	cdc_line_rtsdtr = 0;
	UDINT = 0;
	UDIEN = BIT(EORSTE) | BIT(SOFE) | BIT(SUSPE);
}

int serial_available(void)
{
	uint8_t n = 0;
	uint8_t i;
	uint8_t intr_state = SREG;

	cli();

	if (usb_configuration) {
		UENUM = CDC_RX_ENDPOINT;
		n = UEBCLX;
		if (!n) {
			i = UEINTX;
			if (i & BIT(RXOUTI) && !(i & BIT(RWAL)))
				UEINTX = 0x6B;
		}
	}

	SREG = intr_state;

	if (peek_buf >= 0 && n < 0xFF)
		n++;

	return n;
}

void serial_begin(void)
{
	uint32_t usb_cfg_ms;

	usb_init();

	usb_cfg_ms = millis();
	while (1) {
		if (usb_configuration) {
			_delay_ms(USB_CONFIG_MS);
			return;
		}

		if (usb_suspended) {
			uint32_t usb_susp_ms = millis();

			while (usb_suspended)
				if (millis() - usb_susp_ms > USB_SUSPEND_MS)
					return;
		}

		if (millis() - usb_cfg_ms > USB_TIMEOUT_MS)
			return;
	}
}

int serial_busy(void)
{
	if (usb_configuration)
		return 0;
	else
		return 1;
}

void serial_end(void)
{
	UDIEN = 0;
	UDCON = 1;
	USBCON = 0;
	PLLCSR = 0;

	usb_configuration = 0;
	usb_suspended = 1;

	_delay_ms(25);
}

void serial_flush_input(void)
{
	uint8_t intr_state;

	if (usb_configuration) {
		intr_state = SREG;

		cli();

		UENUM = CDC_RX_ENDPOINT;
		while (UEINTX & BIT(RWAL))
			UEINTX = 0x6B;

		SREG = intr_state;
	}

	peek_buf = -1;
}

void serial_flush_output(void)
{
	uint8_t intr_state = SREG;

	cli();

	if (usb_configuration && transmit_flush_timer) {
		UENUM = CDC_TX_ENDPOINT;
		UEINTX = 0x3A;
		transmit_flush_timer = 0;
	}

	SREG = intr_state;
}

uint32_t serial_get_baud(void)
{
	uint32_t *baud = (uint32_t *) cdc_line_coding;

	return *baud;
}

size_t serial_read(void *ptr, size_t size)
{
	uint8_t *buffer = (uint8_t *) ptr;
	size_t count = 0;
	uint32_t read_ms;
	uint8_t i;
	uint8_t num;
	uint8_t intr_state;

	read_ms = millis();
	if (size <= 0)
		return 0;

	if (peek_buf >= 0) {
		*buffer++ = peek_buf;
		peek_buf = -1;

		size--;
		if (size == 0)
			return 1;

		count = 1;
	}

	do {
		intr_state = SREG;
		cli();

		if (!usb_configuration) {
			SREG = intr_state;
			break;
		}

		UENUM = CDC_RX_ENDPOINT;

		if (!(UEINTX & BIT(RXOUTI))) {
			SREG = intr_state;
			break;
		}

		num = UEBCLX;
		if (num > size)
			num = size;

		for (i = 0; i < num; i++)
			*buffer++ = UEDATX;

		if (!(UEINTX & BIT(RWAL)))
			UEINTX = 0x6B;
		SREG = intr_state;

		count += num;
		size -= num;
		if (size == 0)
			return count;
	} while (millis() - read_ms < SERIAL_TIMEOUT_MS);

	return count;
}

size_t serial_write(const void *ptr, size_t size)
{
	const uint8_t *buffer = (uint8_t *) ptr;
	uint8_t timeout, intr_state, write_size;
	size_t count = 0;

	if (!usb_configuration)
		goto end;

	intr_state = SREG;
	cli();
	UENUM = CDC_TX_ENDPOINT;

	if (transmit_previous_timeout) {
		if (!(UEINTX & BIT(RWAL))) {
			SREG = intr_state;
			goto end;
		}
		transmit_previous_timeout = 0;
	}

	while (size) {
		timeout = UDFNUML + TRANSMIT_TIMEOUT;
		while (1) {
			if (UEINTX & BIT(RWAL))
				break;

			SREG = intr_state;

			if (UDFNUML == timeout) {
				transmit_previous_timeout = 1;
				goto end;
			}

			if (!usb_configuration)
				goto end;

			intr_state = SREG;
			cli();
			UENUM = CDC_TX_ENDPOINT;
		}

		write_size = CDC_TX_SIZE - UEBCLX;
		if (write_size > size)
			write_size = size;

		size -= write_size;
		count += write_size;

		do {
			uint8_t tmp;

			asm volatile(
			"L%=begin:"					"\n\t"
				"ldi	r30, %4"			"\n\t"
				"sub	r30, %3"			"\n\t"
				"cpi	r30, %4"			"\n\t"
				"brsh	L%=err"				"\n\t"
				"lsl	r30"				"\n\t"
				"clr	r31"				"\n\t"
				"subi	r30, lo8(-(pm(L%=table)))"	"\n\t"
				"sbci	r31, hi8(-(pm(L%=table)))"	"\n\t"
				"ijmp"					"\n\t"
			"L%=err:"					"\n\t"
				"rjmp	L%=end"				"\n\t"
			"L%=table:"					"\n\t"
				#if (CDC_TX_SIZE == 64)
				ASM_COPY8("Y+", "X", "%1")
				ASM_COPY8("Y+", "X", "%1")
				ASM_COPY8("Y+", "X", "%1")
				ASM_COPY8("Y+", "X", "%1")
				#endif
				#if (CDC_TX_SIZE >= 32)
				ASM_COPY8("Y+", "X", "%1")
				ASM_COPY8("Y+", "X", "%1")
				#endif
				#if (CDC_TX_SIZE >= 16)
				ASM_COPY8("Y+", "X", "%1")
				#endif
				ASM_COPY8("Y+", "X", "%1")
			"L%=end:"					"\n\t"
				: "+y" (buffer), "=r" (tmp)
				: "x" (&UEDATX), "r" (write_size), "M" (CDC_TX_SIZE)
				: "r30", "r31"
			);
		} while (0);

		if (!(UEINTX & BIT(RWAL)))
			UEINTX = 0x3A;

		transmit_flush_timer = TRANSMIT_FLUSH_TIMEOUT;
	}

	SREG = intr_state;

end:
	return count;
}
