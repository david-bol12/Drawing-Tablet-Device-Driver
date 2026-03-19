#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include "tablet.h"
#include "cdev_controller.h"
#include "ioctl.h"

long tablet_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct button_binding binding;
    struct tablet_event event;

    switch (cmd) {

    case TABLET_SET_BINDING:
        if (copy_from_user(&binding, (void __user *)arg, sizeof(binding)))
            return -EFAULT;
        if (binding.button_id < 1 || binding.button_id > 10)
            return -EINVAL;
        mutex_lock(&bindings_mutex);
        button_bindings[binding.button_id] = binding;
        mutex_unlock(&bindings_mutex);
        return 0;

    case TABLET_GET_SETTING:

        return 0;

    case TABLET_CLR_BINDINGS:
        mutex_lock(&bindings_mutex);
        memset(button_bindings, 0, sizeof(button_bindings));
        mutex_unlock(&bindings_mutex);
        return 0;

    default:
        return -ENOTTY;
    }
}
