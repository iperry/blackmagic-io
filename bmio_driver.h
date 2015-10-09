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
#ifndef BMIO_DRIVER_H
#define BMIO_DRIVER_H

struct bmio_driver;
typedef struct bmio_driver bmio_driver_t;

#include <linux/irqreturn.h>
#include "bm_types.h"
#include "bm_pci.h"
#include "bmio_serial.h"


#include <linux/miscdevice.h>
#include <linux/kref.h>
#include <linux/list.h>
#include <linux/tty.h>

typedef void* IOPCIDevicePtr;
typedef void* BlackmagicIODriverLinuxPtr;


#define DEVICE_NAME_MAX_SIZE 32

struct bmio_driver
{
	bm_pci_device_t* pci;
	IOPCIDevicePtr iopci;
	BlackmagicIODriverLinuxPtr driver;

	int id;
	char name[DEVICE_NAME_MAX_SIZE];

	bmio_serial_t* serial;

	struct miscdevice mdev;
	struct kref ref;
	struct list_head list;
	// Don't put anything down here
};

bmio_driver_t* bmio_driver_alloc(struct pci_dev* pdev);
void bmio_driver_remove(bmio_driver_t* drv);
void bmio_driver_terminate(bmio_driver_t* drv);
int bmio_driver_reboot_callback(struct notifier_block *self, unsigned long val, void *data);


int bmio_driver_init(bmio_driver_t* drv);
void bmio_driver_deinit(bmio_driver_t* drv);

bmio_driver_t* bmio_driver_get(bmio_driver_t* drv);
void bmio_driver_put(bmio_driver_t* drv);

bmio_driver_t* bmio_driver_find_by_inode(struct inode* inode);

const char* bmio_driver_model(bmio_driver_t* drv);
const char* bmio_driver_name(bmio_driver_t* drv);
const char* bmio_driver_device_name(bmio_driver_t* drv);
int bmio_driver_id(bmio_driver_t* drv);

bool bmio_driver_start(bmio_driver_t* drv);
void bmio_driver_stop(bmio_driver_t* drv);

int bmio_driver_suspend(bmio_driver_t* drv);
int bmio_driver_resume(bmio_driver_t* drv);

struct device* bmio_driver_device(bmio_driver_t* drv);


#endif
