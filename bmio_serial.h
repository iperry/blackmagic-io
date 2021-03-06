/* -LICENSE-START-
** Copyright (c) 2013 Blackmagic Design
**
** Permission is hereby granted, free of charge, to any person or organization
** obtaining a copy of the software and accompanying documentation covered by
** this license (the "Software") to use, reproduce, display, distribute,
** execute, and transmit the Software, and to prepare derivative works of the
** Software, and to permit third-parties to whom the Software is furnished to
** do so, all subject to the following:
** 
** The copyright notices in the Software and this entire statement, including
** the above license grant, this restriction and the following disclaimer,
** must be included in all copies of the Software, in whole or in part, and
** all derivative works of the Software, unless such copies or derivative
** works are solely in the form of machine-executable object code generated by
** a source language processor.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
** SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
** FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
** -LICENSE-END-
*/
#ifndef BMIO_SERIAL_H
#define BMIO_SERIAL_H

struct bmio_serial;
typedef struct bmio_serial bmio_serial_t;

#include "bmio_driver.h"

#include <linux/tty.h>
#include <linux/version.h>

struct bmio_driver;

typedef void (*bmio_serial_rx_callback_t)(struct bmio_driver*);

struct bmio_serial
{
	bmio_serial_rx_callback_t rx_callback;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
	struct tty_port port;
#else
	struct tty_struct* tty;
#endif
};

int bmio_serial_driver_init(void);
void bmio_serial_driver_exit(void);

int bmio_serial_probe(bmio_driver_t* drv);
void bmio_serial_remove(bmio_driver_t* drv);

void bmio_serial_port_set_rx_callback(bmio_driver_t* drv, bmio_serial_rx_callback_t callback);

bool bmio_serial_supported(bmio_driver_t* drv);

int bmio_serial_open(bmio_driver_t* drv);
void bmio_serial_close(bmio_driver_t* drv);

int bmio_serial_write(bmio_driver_t* drv, const void* data, uint32_t length);
int bmio_serial_write_room(bmio_driver_t* drv);
int bmio_serial_chars_in_buffer(bmio_driver_t* drv);

bool bmio_serial_pop_rx_byte(bmio_driver_t* drv, unsigned char* byte);

int bmio_serial_get_device_path(bmio_driver_t* drv, char* buffer, size_t len);


#endif
