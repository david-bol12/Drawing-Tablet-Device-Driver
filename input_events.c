//
// Created by david-bol12 on 3/13/26.
//

#include "input_events.h"
#include <linux/input.h>
#include "tablet.h"
#include <linux/module.h>
#include <linux/kernel.h>

#define MAX_BUTTONS 11
struct button_binding button_bindings[MAX_BUTTONS];
DEFINE_MUTEX(bindings_mutex);
static struct input_dev *tablet_input_dev;

static int input_dev_init(void) {

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
    for (int i = 0; i < KEY_MAX; i++)
        set_bit(i, tablet_input_dev->keybit);

    if (input_register_device(tablet_input_dev)) {
        printk(KERN_ERR "tablet: failed to register input device\n");
        input_free_device(tablet_input_dev);
        tablet_input_dev = NULL;
        return -ENODEV;
    }

    // default button bindings - can be overridden at runtime via ioctl
    button_bindings[1]  = (struct button_binding){ 1,  KEY_Z,         MOD_CTRL  };  // Ctrl+Z
    button_bindings[2]  = (struct button_binding){ 2,  KEY_C,         MOD_CTRL  };  // Ctrl+C
    button_bindings[3]  = (struct button_binding){ 3,  KEY_V,         MOD_CTRL  };  // Ctrl+V
    button_bindings[4]  = (struct button_binding){ 4,  KEY_S,         MOD_CTRL  };  // Ctrl+S
    button_bindings[5]  = (struct button_binding){ 5,  KEY_Y,         MOD_CTRL  };  // Ctrl+Y
    button_bindings[6]  = (struct button_binding){ 6,  KEY_MINUS,     MOD_CTRL  };  // Ctrl+-
    button_bindings[7]  = (struct button_binding){ 7,  KEY_EQUAL,     MOD_CTRL  };  // Ctrl+=  (zoom in)
    button_bindings[8]  = (struct button_binding){ 8,  KEY_CAPSLOCK,  0         };  // Caps Lock
    button_bindings[9]  = (struct button_binding){ 9,  KEY_VOLUMEUP,  0         };  // Volume Up
    button_bindings[10] = (struct button_binding){ 10, KEY_VOLUMEDOWN,0         };  // Volume Down

    return 0;
}

static void press_binding(struct button_binding *b) {
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
}

static void release_binding(struct button_binding *b) {
    input_report_key(tablet_input_dev, b->keycode, 0);

    // release modifiers
    if (b->modifiers & MOD_CTRL)
        input_report_key(tablet_input_dev, KEY_LEFTCTRL, 0);
    if (b->modifiers & MOD_ALT)
        input_report_key(tablet_input_dev, KEY_LEFTALT, 0);
    if (b->modifiers & MOD_SHIFT)
        input_report_key(tablet_input_dev, KEY_LEFTSHIFT, 0);

    input_sync(tablet_input_dev);
}