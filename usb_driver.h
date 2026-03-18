//
// Created by david-bol12 on 3/6/26.
//

#ifndef DEVICEDRIVERPROJECT_USB_DRIVER_H
#define DEVICEDRIVERPROJECT_USB_DRIVER_H

#include <linux/usb.h>
#include <linux/input.h>
#include <linux/types.h>

#define VENDOR_ID  0x28bd // Wacom: 056a
#define PRODUCT_ID 0x0937 // Wacom: 0376

#define TABLET_MAX_X      32768 // Wacom: 15200 
#define TABLET_MAX_Y      32768 // Wacom: 9500
#define TABLET_MAX_PRESSURE 8191

struct tablet_usb_dev {
    struct usb_device *usb_dev;
    struct usb_interface *interface;
    unsigned char *buf;
    struct urb *urb;
    __u8 int_ep;
    size_t buf_size;
    struct tablet_event *tablet_data;
    struct input_dev *pen_input_dev;
    char phys[64];
    struct input_dev *button_input_dev;
};

#endif //DEVICEDRIVERPROJECT_USB_DRIVER_H