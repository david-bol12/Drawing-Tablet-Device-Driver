#include <linux/init.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/usb.h>
#include "data_parsing.h"
#include "usb_driver.h"

#include "cdev_controller.h"
#include "input_events.h"
#include "tablet.h"

MODULE_LICENSE("Dual BSD/GPL");

void handle_button_input(struct tablet_usb_dev *dev);
void handle_pen_input(struct tablet_usb_dev *dev);

struct tablet_event *tablet_data;

static const struct usb_device_id tablet_table[] = {
    { USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
    { }
};

MODULE_DEVICE_TABLE(usb, tablet_table);

static void tablet_irq_callback(struct urb *urb)
{
	struct tablet_usb_dev *dev = urb->context;

	if (urb->status == 0) {
		if (dev->buf[0] == 6) { // Button Input
			handle_button_input(dev);
		} else if (dev->buf[0] == 7) { // Wacom: 10, Pen Input
			handle_pen_input(dev);
		}
		for (int i = 0; i < urb->actual_length; i++)
			printk(KERN_CONT " %02x", dev->buf[i]);
		printk("\n");
		cdev_buffer_write(dev->tablet_data);

	}

	// Submit urb again to receive more data
	usb_submit_urb(urb, GFP_ATOMIC);
}


static int tablet_probe(struct usb_interface *interface, const struct usb_device_id *id)
// Entry point for usb device
{
	struct tablet_usb_dev *dev;
	struct usb_host_interface *interface_desc;
	struct usb_endpoint_descriptor *endpoint;

	interface_desc = interface->cur_altsetting;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->usb_dev = usb_get_dev(interface_to_usbdev(interface));

	dev->tablet_data = tablet_data;

	/*
	 The tablet actually has three interfaces each having an int in endpoint and the third also having an int out endpoint.

	 I've discovered that the first is for button data, the second is for pen data and I can't figure out what the third does.

	 Could potentially make a separate driver for each interface.
	 */

	dev->interface = interface;

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

	switch (interface_desc->desc.bInterfaceNumber) {
		case PEN_INTERFACE:
			dev->pen_input_dev = input_allocate_device();
			dev->pen_input_dev->dev.parent = &interface->dev;
			cursor_control_init(dev);
			if (input_register_device(dev->pen_input_dev)) {
				goto error;
			}
			if (!dev->pen_input_dev) {
				goto error;
			}
			break;
		case BUTTON_INTERFACE:
			dev->button_input_dev = input_allocate_device();
			if (button_dev_init(dev->button_input_dev)) {
				goto error;
			}
			if (!dev->button_input_dev) {
				goto error;
			}
			break;
	}

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
	struct tablet_usb_dev *dev = usb_get_intfdata(interface);


	usb_kill_urb(dev->urb);
	usb_free_urb(dev->urb);
	switch (dev->interface->minor) {
		case PEN_INTERFACE:
			input_unregister_device(dev->pen_input_dev);
			break;
		case BUTTON_INTERFACE:
			input_unregister_device(dev->button_input_dev);
			break;
	}
	kfree(dev->buf);
	usb_put_dev(dev->usb_dev);
	kfree(dev);

	printk(KERN_INFO "Tablet Disconnected\n");
}

void handle_button_input(struct tablet_usb_dev *dev) {
	struct button_array prev_pressed = dev->tablet_data->tab_buttons;
	get_buttons_pressed(dev->buf, dev->urb->actual_length, &dev->tablet_data->tab_buttons);
	printk(KERN_ALERT "Button(s) ");
	if (dev->tablet_data->tab_buttons.no_pressed == 0) {
		printk(KERN_ALERT "Released \n");
	} else {
		for (int i = 0; i < dev->tablet_data->tab_buttons.no_pressed; i++) {
			printk(KERN_ALERT "%d, ", dev->tablet_data->tab_buttons.buttons[i]);
		}
		printk(KERN_ALERT "Pressed \n");
	}
	update_button_states(&dev->tablet_data->tab_buttons, dev->button_input_dev);
}

void handle_pen_input(struct tablet_usb_dev *dev) {

	update_pen_data(dev->buf, dev->urb->actual_length, dev->tablet_data);
	// Report pen coordinates
	cursor_control_reporting(dev, *tablet_data, dev->tablet_data->pen_in_range);
	
}
    

static struct usb_driver tablet_usb_driver = {
	.name = "custom_tablet_driver",
	.probe = tablet_probe,
	.disconnect = tablet_disconnect,
	.id_table = tablet_table,
};

static int __init device_module_init(void)
{

	tablet_data = kzalloc(sizeof(struct tablet_event), GFP_KERNEL);

	int result = tablet_cdev_init();

	if (result != 0) {
		pr_err("CDEV INIT FAILED");
		return -1;
	}

	return usb_register(&tablet_usb_driver);
}

module_init(device_module_init);

static void __exit driver_module_cleanup(void)
{
	tablet_cdev_cleanup();
	usb_deregister(&tablet_usb_driver);
}

module_exit(driver_module_cleanup);