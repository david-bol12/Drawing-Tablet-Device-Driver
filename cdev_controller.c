#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/wait.h>        
#include <linux/uaccess.h>
#include "tablet.h"
#include "cdev_controller.h"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Team 12");
MODULE_DESCRIPTION("Drawing Tablet Driver");

#define DEVICE_NAME "tablet"
#define CLASS_NAME "tablet_class"
#define BUFFER_SIZE 4096

static int major_number;
static struct class *tablet_class;
static struct device *tablet_device;
static struct cdev tablet_cdev;

// tracks how many processes have the device open
static int open_count = 0;

// Circular buffer
static struct tablet_event event_buffer[BUFFER_SIZE];
static int buf_head = 0;
static int buf_tail = 0;
static int buf_count = 0;

// Initiialise mutex for buffer
static DEFINE_MUTEX(tablet_mutex);

// Initialise wait queue - processes sleep here when buffer is empty
static DECLARE_WAIT_QUEUE_HEAD(read_queue);
static DECLARE_WAIT_QUEUE_HEAD(write_queue);

static int tablet_open(struct inode *inode, struct file *file);
static int tablet_release(struct inode *inode, struct file *file);
static ssize_t tablet_read(struct file *file, char __user *user_buf, size_t count, loff_t *offset);
static ssize_t tablet_write(struct file *file, const char __user *user_buf, size_t count, loff_t *offset);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = tablet_open, 
    .release = tablet_release,
    .read = tablet_read,
    .write = tablet_write,
};

static int tablet_open(struct inode *inode, struct file *file) {
    open_count++;

    if (open_count == 1) {
        mutex_lock(&tablet_mutex);
        buf_head = 0;
        buf_tail = 0;
        buf_count = 0;
        mutex_unlock(&tablet_mutex);
        printk(KERN_INFO "tablet: buffer initialised\n");
    }
    printk(KERN_INFO "tablet: device opened (open count: %d)\n", open_count);
    return 0;

}

static int tablet_release(struct inode *inode, struct file *file) {
    open_count--;
    // wake up sleeping readers so they don't sleep forever
    if (open_count == 0) {
        wake_up_interruptible(&read_queue);
        wake_up_interruptible(&write_queue);
    }
    printk(KERN_INFO "tablet: device closed (open count: %d)\n", open_count);
    return 0;
}

static ssize_t tablet_read(struct file *file, char __user *user_buf, size_t count, loff_t *offset) {
    struct tablet_event event;

    // check if userspace gave enough space for one event
    if (count < sizeof(struct tablet_event)) {
        return -EINVAL;
    }

    // sleep until buffer has data or device is closed
    if (wait_event_interruptible(read_queue, buf_count > 0 || open_count == 0)) {
        return -ERESTARTSYS;
    }  
    
    // check if device was closed while asleep
    if (buf_count == 0) {
        return -EIO;
    }

    mutex_lock(&tablet_mutex);
    event = event_buffer[buf_tail];
    buf_tail = (buf_tail + 1) % BUFFER_SIZE;
    buf_count--;
    mutex_unlock(&tablet_mutex);

    wake_up_interruptible(&write_queue);

    if (copy_to_user(user_buf, &event, sizeof(event))) {
        return -EFAULT;
    }

    return sizeof(event);
}

static ssize_t tablet_write(struct file *file, const char __user *user_buf, size_t count, loff_t *offset) {
    struct tablet_event event;


    // check if userspace gave enough space for one event
    if (count < sizeof(struct tablet_event)) {
        return -EINVAL;
    }


    if (copy_from_user(&event, user_buf, sizeof(event))) {
        return -EFAULT;
    }

    if (wait_event_interruptible(write_queue, buf_count < BUFFER_SIZE || open_count == 0)) {
        return -ERESTARTSYS;
    }

    if (open_count == 0) {
        return -EIO;
    }

    mutex_lock(&tablet_mutex);
    event_buffer[buf_head] = event;
    buf_head = (buf_head + 1) % BUFFER_SIZE;
    buf_count++;
    mutex_unlock(&tablet_mutex);

    wake_up_interruptible(&read_queue);

    return sizeof(event);

    
}

int tablet_buffer_write(struct tablet_event *event) {
    if (buf_count >= BUFFER_SIZE) {
        printk(KERN_WARNING "tablet: buffer full, dropping event\n");
        return -1;
    }

    mutex_lock(&tablet_mutex);
    event_buffer[buf_head] = *event;
    buf_head = (buf_head + 1) % BUFFER_SIZE;
    buf_count++;
    mutex_unlock(&tablet_mutex);

    wake_up_interruptible(&read_queue);

    return 0;
}
EXPORT_SYMBOL(tablet_buffer_write);

int tablet_buffer_read(struct tablet_event *event) {
    if (buf_count == 0) {
        return -1;
    }

    mutex_lock(&tablet_mutex);
    *event = event_buffer[buf_tail];
    buf_tail = (buf_tail + 1) % BUFFER_SIZE;
    buf_count--;
    mutex_unlock(&tablet_mutex);

    wake_up_interruptible(&write_queue);

    return 0;
}
EXPORT_SYMBOL(tablet_buffer_read);

int tablet_init(void) {

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

void tablet_exit(void) {
    device_destroy(tablet_class, MKDEV(major_number, 0));
    class_destroy(tablet_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_ALERT "tablet: /dev/tablet removed\n");
}

// Currently the module is mounted in the usb driver. Can be uncommented for testing but don't commit with it.

// module_init(tablet_init);
// module_exit(tablet_exit);


