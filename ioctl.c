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

    case TABLET_CLR_BUFFER:
        cdev_buffer_clear();
        return 0;

    case TABLET_INJECT_EVENT:
        if (copy_from_user(&event, (void __user *)arg, sizeof(event)))
            return -EFAULT;
        return cdev_buffer_write(&event);

    case TABLET_GET_EVENT:
        if (cdev_buffer_read(&event) < 0)
            return -EAGAIN;
        if (copy_to_user((void __user *)arg, &event, sizeof(event)))
            return -EFAULT;
        return 0;

    case TABLET_SET_BINDING:
        if (copy_from_user(&binding, (void __user *)arg, sizeof(binding)))
            return -EFAULT;
        if (binding.button_id < 1 || binding.button_id > 10)
            return -EINVAL;
        mutex_lock(&bindings_mutex);
        button_bindings[binding.button_id] = binding;
        mutex_unlock(&bindings_mutex);
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
        return 0;

    default:
        return -ENOTTY;
    }
}
