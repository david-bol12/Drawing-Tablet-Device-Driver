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
#include "proc_file_controller.h"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Team 12");
MODULE_DESCRIPTION("Drawing Tablet Driver");

#define DEVICE_NAME "Tablet Character Device"
#define CLASS_NAME "tablet_class"

static int major_number;
static struct class *tablet_class;
static struct device *tablet_device;

// tracks how many processes have the device open
 int open_count = 0;

static long data_instance = 0;

// Event slot
struct tablet_event event_buffer;


// Initiialise mutex for buffer
DEFINE_MUTEX(tablet_mutex);

// Initialise wait queue - processes waiting for a new data_instance
static DECLARE_WAIT_QUEUE_HEAD(read_queue);

static int cdev_open(struct inode *inode, struct file *file);
static int cdev_release(struct inode *inode, struct file *file);
static ssize_t cdev_read(struct file *file, char __user *user_buf, size_t count, loff_t *offset);
static ssize_t cdev_write(struct file *file, const char __user *user_buf, size_t count, loff_t *offset);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = cdev_open,
    .release = cdev_release,
    .read = cdev_read,
    .write = cdev_write,
};

//for proc/ file
int total_reads = 0;
int total_writes = 0;


// A struct that contains unique data for each instance that is reading from the cdev

struct reader_data {
    unsigned long instance_no;
};

static int cdev_open(struct inode *inode, struct file *file) {
    open_count++;

    struct reader_data *reader_data = kzalloc(sizeof(struct reader_data), GFP_KERNEL);

    file->private_data = reader_data;

    printk(KERN_INFO "tablet: device opened (open count: %d)\n", open_count);
    return 0;

}

static int cdev_release(struct inode *inode, struct file *file) {
    open_count--;
    // wake up sleeping readers so they don't sleep forever
    if (open_count == 0) {
        wake_up_interruptible(&read_queue);
    }

    kfree(file->private_data);
    printk(KERN_INFO "tablet: device closed (open count: %d)\n", open_count);
    return 0;
}

static ssize_t cdev_read(struct file *file, char __user *user_buf, size_t count, loff_t *offset) {
    struct tablet_event event;

    struct reader_data *reader_data = file->private_data;

    reader_data->instance_no = data_instance;

    // check if userspace gave enough space for one event
    if (count < sizeof(struct tablet_event)) {
        return -EINVAL;
    }

    // sleep until buffer has data or device is closed
    if (wait_event_interruptible(read_queue, reader_data->instance_no < data_instance)) {
        return -ERESTARTSYS;
    }


    mutex_lock(&tablet_mutex);
    event = event_buffer;
    reader_data->instance_no = data_instance;
    total_reads++;
    mutex_unlock(&tablet_mutex);


    if (copy_to_user(user_buf, &event, sizeof(event))) {
        return -EFAULT;
    }

    return sizeof(event);
}


static ssize_t cdev_write(struct file *file, const char __user *user_buf, size_t count, loff_t *offset) {
    struct tablet_event event;


    // check if userspace gave enough space for one event
    if (count < sizeof(struct tablet_event)) {
        return -EINVAL;
    }


    if (copy_from_user(&event, user_buf, sizeof(event))) {
        return -EFAULT;
    }

    //no sleep needed in write

    mutex_lock(&tablet_mutex);
    event_buffer = event;
    //increment to wake up reader
    data_instance++;
    mutex_unlock(&tablet_mutex);

    wake_up_interruptible(&read_queue);

    return sizeof(event);

    
}

int cdev_buffer_write(struct tablet_event *event) {

    mutex_lock(&tablet_mutex);
    event_buffer = *event;
    data_instance++;
    total_writes++;
    mutex_unlock(&tablet_mutex);

    wake_up_interruptible(&read_queue);

    return 0;
}
EXPORT_SYMBOL(cdev_buffer_write);

int cdev_buffer_read(struct tablet_event *event) {

    mutex_lock(&tablet_mutex);
    *event = event_buffer;
    mutex_unlock(&tablet_mutex);

    return 0;
}
EXPORT_SYMBOL(cdev_buffer_read);

int tablet_cdev_init(void) {

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

    if (proc_init() != 0) {
        device_destroy(tablet_class, MKDEV(major_number, 0));
        class_destroy(tablet_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return -ENOMEM;
    }

    return 0;
}

void tablet_cdev_cleanup(void) {
    proc_exit();
    device_destroy(tablet_class, MKDEV(major_number, 0));
    class_destroy(tablet_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_ALERT "tablet: /dev/tablet removed\n");
}


