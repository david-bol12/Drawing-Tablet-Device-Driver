#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/wait.h>        
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/input.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/atomic.h>
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

// stats counters - atomic so they're safe to increment from anywhere
static atomic_t total_reads  = ATOMIC_INIT(0);
static atomic_t total_writes = ATOMIC_INIT(0);

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
static long tablet_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations fops = {
    .owner          = THIS_MODULE,
    .open           = tablet_open,
    .release        = tablet_release,
    .read           = tablet_read,
    .write          = tablet_write,
    .unlocked_ioctl = tablet_ioctl,
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

    atomic_inc(&total_reads);
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

    atomic_inc(&total_writes);
    return sizeof(event);
}

int tablet_buffer_write(struct tablet_event *event) {
    struct button_binding b;

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

    // if a button was pressed, look up its binding and fire it
    if (event->button >= 1 && event->button <= 10) {
        mutex_lock(&bindings_mutex);
        b = button_bindings[event->button];
        mutex_unlock(&bindings_mutex);
        fire_binding(&b);
    }

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

// Virtual keyboard input device - used to inject keypresses into the OS
static struct input_dev *tablet_input_dev;

// Button binding table - buttons are numbered 1-10, slot 0 unused
#define MAX_BUTTONS 11
static struct button_binding button_bindings[MAX_BUTTONS];
static DEFINE_MUTEX(bindings_mutex);

// Fires the key combination stored in a binding (e.g. Ctrl+Z)
// Press modifiers → press key → release key → release modifiers
static void fire_binding(struct button_binding *b) {
    if (!tablet_input_dev || b->keycode == 0)
        return;

    // press modifiers
    if (b->modifiers & MOD_CTRL)
        input_report_key(tablet_input_dev, KEY_LEFTCTRL, 1);
    if (b->modifiers & MOD_ALT)
        input_report_key(tablet_input_dev, KEY_LEFTALT, 1);
    if (b->modifiers & MOD_SHIFT)
        input_report_key(tablet_input_dev, KEY_LEFTSHIFT, 1);

    // press and release the main key
    input_report_key(tablet_input_dev, b->keycode, 1);
    input_sync(tablet_input_dev);
    input_report_key(tablet_input_dev, b->keycode, 0);
    input_sync(tablet_input_dev);

    // release modifiers
    if (b->modifiers & MOD_CTRL)
        input_report_key(tablet_input_dev, KEY_LEFTCTRL, 0);
    if (b->modifiers & MOD_ALT)
        input_report_key(tablet_input_dev, KEY_LEFTALT, 0);
    if (b->modifiers & MOD_SHIFT)
        input_report_key(tablet_input_dev, KEY_LEFTSHIFT, 0);

    input_sync(tablet_input_dev);
    printk(KERN_INFO "tablet: fired keycode %d modifiers %d\n",
           b->keycode, b->modifiers);
}

static long tablet_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct button_binding binding;

    switch (cmd) {

    case TABLET_SET_BINDING:
        if (copy_from_user(&binding, (void __user *)arg, sizeof(binding)))
            return -EFAULT;
        if (binding.button_id < 1 || binding.button_id > 10)
            return -EINVAL;
        mutex_lock(&bindings_mutex);
        button_bindings[binding.button_id] = binding;
        mutex_unlock(&bindings_mutex);
        printk(KERN_INFO "tablet: button %d bound to keycode %d modifiers %d\n",
               binding.button_id, binding.keycode, binding.modifiers);
        return 0;

    case TABLET_GET_BINDING:
        if (copy_from_user(&binding, (void __user *)arg, sizeof(binding)))
            return -EFAULT;
        if (binding.button_id < 1 || binding.button_id > 10)
            return -EINVAL;
        mutex_lock(&bindings_mutex);
        binding = button_bindings[binding.button_id];
        mutex_unlock(&bindings_mutex);
        if (copy_to_user((void __user *)arg, &binding, sizeof(binding)))
            return -EFAULT;
        return 0;

    case TABLET_CLR_BINDINGS:
        mutex_lock(&bindings_mutex);
        memset(button_bindings, 0, sizeof(button_bindings));
        mutex_unlock(&bindings_mutex);
        printk(KERN_INFO "tablet: all bindings cleared\n");
        return 0;

    default:
        return -ENOTTY;
    }
}

// /proc/tablet_stats — shows live driver statistics
static int tablet_proc_show(struct seq_file *m, void *v) {
    seq_printf(m, "total_reads:  %d\n", atomic_read(&total_reads));
    seq_printf(m, "total_writes: %d\n", atomic_read(&total_writes));
    mutex_lock(&tablet_mutex);
    seq_printf(m, "buffer_count: %d\n", buf_count);
    mutex_unlock(&tablet_mutex);
    return 0;
}

static int tablet_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, tablet_proc_show, NULL);
}

static const struct proc_ops tablet_proc_ops = {
    .proc_open    = tablet_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

int tablet_init(void) {
    int i;

    // Create virtual keyboard so we can inject keypresses into the OS
    tablet_input_dev = input_allocate_device();
    if (!tablet_input_dev) {
        printk(KERN_ERR "tablet: failed to allocate input device\n");
        return -ENOMEM;
    }
    tablet_input_dev->name = "Tablet Button Virtual Keyboard";
    tablet_input_dev->id.bustype = BUS_VIRTUAL;

    // declare that this device sends key events
    set_bit(EV_KEY, tablet_input_dev->evbit);

    // declare support for all keys so any binding will work
    for (i = 0; i < KEY_MAX; i++)
        set_bit(i, tablet_input_dev->keybit);

    if (input_register_device(tablet_input_dev)) {
        printk(KERN_ERR "tablet: failed to register input device\n");
        input_free_device(tablet_input_dev);
        tablet_input_dev = NULL;
        return -ENODEV;
    }
    printk(KERN_ALERT "tablet: virtual keyboard registered\n");

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

    // create /proc/tablet_stats
    proc_create("tablet_stats", 0, NULL, &tablet_proc_ops);
    printk(KERN_ALERT "tablet: /proc/tablet_stats created\n");

    return 0;
}

void tablet_exit(void) {
    remove_proc_entry("tablet_stats", NULL);
    device_destroy(tablet_class, MKDEV(major_number, 0));
    class_destroy(tablet_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    if (tablet_input_dev)
        input_unregister_device(tablet_input_dev);
    printk(KERN_ALERT "tablet: /dev/tablet removed\n");
}

// Currently the module is mounted in the usb driver. Can be uncommented for testing but don't commit with it.

// module_init(tablet_init);
// module_exit(tablet_exit);


