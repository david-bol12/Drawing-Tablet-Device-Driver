/*                                                     
 * $Id: hello.c,v 1.5 2004/10/26 03:32:21 corbet Exp $ 
 */                                                    
#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>

MODULE_LICENSE("Dual BSD/GPL");

#define VENDOR_ID  0x28bd  // example
#define PRODUCT_ID 0x0937

static const struct usb_device_id tablet_table[] = {
	{ USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{ }
};

MODULE_DEVICE_TABLE(usb, tablet_table);


struct tablet_dev {
	struct usb_device *udev;
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
		for (i = 0; i < urb->actual_length; i++)
			printk(KERN_CONT " %02x", dev->buf[i]);
		printk(KERN_CONT "\n");
	}

	/* Resubmit URB to keep receiving data */
	usb_submit_urb(urb, GFP_ATOMIC);
}

static int tablet_probe(struct usb_interface *interface,
						 const struct usb_device_id *id)
{
	struct tablet_dev *dev;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	int i;

	iface_desc = interface->cur_altsetting;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->udev = usb_get_dev(interface_to_usbdev(interface));
	dev->interface = interface;

	/* Find interrupt IN endpoint */
	for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
		endpoint = &iface_desc->endpoint[i].desc;

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

	usb_fill_int_urb(
		dev->urb,
		dev->udev,
		usb_rcvintpipe(dev->udev, dev->int_ep),
		dev->buf,
		dev->buf_size,
		tablet_irq_callback,
		dev,
		endpoint->bInterval
	);

	usb_set_intfdata(interface, dev);
	usb_submit_urb(dev->urb, GFP_KERNEL);

	printk(KERN_INFO "Raw tablet driver bound\n");
	return 0;

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
	usb_put_dev(dev->udev);
	kfree(dev);

	printk(KERN_INFO "Raw tablet driver disconnected\n");
}

static struct usb_driver tablet_driver = {
	.name = "raw_usb_tablet",
	.probe = tablet_probe,
	.disconnect = tablet_disconnect,
	.id_table = tablet_table,
};

module_usb_driver(tablet_driver);
