#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>
#include "data_parsing.h"
#include "usb_driver.h"

MODULE_LICENSE("Dual BSD/GPL");

#define VENDOR_ID  0x28bd
#define PRODUCT_ID 0x0937

static const struct usb_device_id tablet_table[] = {
    { USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
    { }
};

MODULE_DEVICE_TABLE(usb, tablet_table);


struct tablet_dev {
    struct usb_device *usb_dev;
    struct usb_interface *interface;
    unsigned char *buf;
    struct urb *urb;
    __u8 int_ep;
    size_t buf_size;
};

static void tablet_irq_callback(struct urb *urb)
{
	struct tablet_dev *dev = urb->context;
	int i;

	if (urb->status == 0) {
		printk(KERN_INFO "tablet data:");
		if (dev->buf[0] == 6) {
			char buttons[7];
			struct button_array pressed = {
				0,
				buttons
			};
			get_buttons_pressed(dev->buf, urb->actual_length, &pressed);
			printk(KERN_ALERT "Button(s) ");
			if (pressed.no_pressed == 0) {
				printk(KERN_ALERT "Released \n");
			} else {
				for (i = 0; i < pressed.no_pressed; i++) {
					printk(KERN_ALERT "%d, ", pressed.buttons[i]);
				}
				printk(KERN_ALERT "Pressed \n");
			}
		}
		for (i = 0; i < urb->actual_length; i++)
			printk(KERN_CONT " %02x", dev->buf[i]);

		printk(KERN_CONT "\n");
	}

	// Submit urb again to receive more data
	usb_submit_urb(urb, GFP_ATOMIC);
}


static int tablet_probe(struct usb_interface *interface, const struct usb_device_id *id)
// Entry point for usb device
{
	struct tablet_dev *dev;
	struct usb_host_interface *interface_desc;
	struct usb_endpoint_descriptor *endpoint;

	interface_desc = interface->cur_altsetting;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->usb_dev = usb_get_dev(interface_to_usbdev(interface));

	/*
	 The tablet actually has three interfaces each having an int in endpoint and the third also having an int out endpoint.

	 I've discovered that the first is for button data, the second is for pen data and I can't figure out what the third does.

	 Could potentially make a separate driver for each interface.
	 */

	dev->interface = interface;

	// Find interrupt IN endpoint
	printk(KERN_ALERT "%d", interface_desc->desc.bNumEndpoints);

	for (int i = 0; i < interface_desc->desc.bNumEndpoints; i++) {
		endpoint = &interface_desc->endpoint[i].desc;

		if (usb_endpoint_is_int_in(endpoint)) {
			dev->int_ep = endpoint->bEndpointAddress;
			dev->buf_size = usb_endpoint_maxp(endpoint);
			break;
		}
	}

	if (!dev->int_ep) {
		printk(KERN_ERR "No interrupt IN endpoint found\n");
		goto error;
	}

	dev->buf = kmalloc(dev->buf_size, GFP_KERNEL);
	dev->urb = usb_alloc_urb(0, GFP_KERNEL);

	// Initialises URB
	usb_fill_int_urb(
		dev->urb,
		dev->usb_dev,
		usb_rcvintpipe(dev->usb_dev, dev->int_ep),
		dev->buf,
		dev->buf_size,
		tablet_irq_callback,
		dev,
		endpoint->bInterval
	);

	usb_set_intfdata(interface, dev);

	usb_submit_urb(dev->urb, GFP_KERNEL);

	printk(KERN_ALERT "Raw tablet driver bound\n");
	return 0;

	// If there is an error jump here and free to prevent mem leak
	error:
		kfree(dev);
	return -ENODEV;
}

static void tablet_disconnect(struct usb_interface *interface)
{
	struct tablet_dev *dev = usb_get_intfdata(interface);

	usb_kill_urb(dev->urb);
	usb_free_urb(dev->urb);
	kfree(dev->buf);
	usb_put_dev(dev->usb_dev);
	kfree(dev);

	printk(KERN_INFO "Tablet Disconnected\n");
}

static struct usb_driver tablet_driver = {
	.name = "custom_tablet_driver",
	.probe = tablet_probe,
	.disconnect = tablet_disconnect,
	.id_table = tablet_table,
};

module_usb_driver(tablet_driver);