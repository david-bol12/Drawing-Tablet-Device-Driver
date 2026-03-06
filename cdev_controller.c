#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Team 12");
MODULE_DESCRIPTION("Drawing Tablet Driver");

#define DEVICE_NAME "tablet"
#define CLASS_NAME "tablet_class"

static int major_number;
static struct class *tablet_class;
static struct device *tablet_device;
static struct cdev tablet_cdev;

// tracks how many processes have the device open
static int open_count = 0;

static int tablet_open(struct inode *inode, struct file *file);
static int tablet_release(struct inode *inode, struct file *file);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = tablet_open, 
    .release = tablet_release,
};

static int tablet_open(struct inode *inode, struct file *file) {
    open_count++;
    printk(KERN_INFO "tablet: device opened (open count: %d)\n", open_count);
    return 0;

}

static int tablet_release(struct inode *inode, struct file *file) {
    open_count--;
    printk(KERN_INFO "tablet: device closed (open count: %d)\n", open_count);
    return 0;
}

static int __init tablet_init(void) {

    // Registers driver with kernel as character device
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ERR "tablet: failed to get major number\n");
        return major_number;
    }
    printk(KERN_ALERT "tablet: got major number %d\n", major_number);

    // Creates entry in /sys/class/tablet_class/
    tablet_class = class_create(CLASS_NAME);
    if (IS_ERR(tablet_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "tablet: failed to create class\n");
        return PTR_ERR(tablet_class);
    }
    printk(KERN_ALERT "tablet: device class created\n");

    // triggers /dev/tablet to be made, MKDEV() combines major and minor numbers into a single dev_t value the kernel uses to identify device
    tablet_device = device_create(tablet_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if(IS_ERR(tablet_device)) {
        class_destroy(tablet_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "tablet: failed to create device\n");
        return PTR_ERR(tablet_device);
    }
    printk(KERN_ALERT "tablet: /dev/tablet created\n");

    return 0;
}

static void __exit tablet_exit(void) {
    device_destroy(tablet_class, MKDEV(major_number, 0));
    class_destroy(tablet_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_ALERT "tablet: /dev/tablet removed\n");
}

module_init(tablet_init);
module_exit(tablet_exit);


