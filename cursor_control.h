#ifndef CURSOR_CONTROL_H
#define CURSOR_CONTROL_H

#include <linux/usb.h>
#include <linux/input.h>
#include <linux/types.h>

#include "usb_driver.h"

void cursor_control_reporting(struct tablet_usb_dev *dev, unsigned char *buf, short x, short y, int pressure);
void cursor_control_initialize(struct tablet_usb_dev *dev);

#endif // CURSOR_CONTROL_H