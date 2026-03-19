//
// Created by david-bol12 on 3/13/26.
//

#include "input_events.h"
#include <linux/input.h>
#include "tablet.h"
#include <linux/module.h>
#include <linux/kernel.h>

#define TABLET_RES_X 200
#define TABLET_RES_Y 200

static struct button_binding button_bindings[MAX_BUTTONS];

int button_dev_init(struct input_dev *button_input_dev) {

    // Create virtual keyboard so we can inject keypresses into the OS
    if (!button_input_dev) {
        printk(KERN_ERR "tablet: failed to allocate input device\n");
        return -ENOMEM;
    }
    button_input_dev->name = "Tablet Button Virtual Keyboard";
    button_input_dev->id.bustype = BUS_VIRTUAL;

    // declare that this device sends key events
    set_bit(EV_KEY, button_input_dev->evbit);

    // declare support for all keys so any binding will work
    for (int i = 0; i < KEY_MAX; i++)
        set_bit(i, button_input_dev->keybit);

    if (input_register_device(button_input_dev)) {
        printk(KERN_ERR "tablet: failed to register input device\n");
        input_free_device(button_input_dev);
        button_input_dev = NULL;
        return -ENODEV;
    }

    // default button bindings - can be overridden at runtime via ioctl
    button_bindings[0] = (struct button_binding){ KEY_VOLUMEDOWN,0         };  // Volume Down
    button_bindings[1]  = (struct button_binding){KEY_Z,         MOD_CTRL  };  // Ctrl+Z
    button_bindings[2]  = (struct button_binding){KEY_C,         MOD_CTRL  };  // Ctrl+C
    button_bindings[3]  = (struct button_binding){KEY_V,         MOD_CTRL  };  // Ctrl+V
    button_bindings[4]  = (struct button_binding){KEY_S,         MOD_CTRL  };  // Ctrl+S
    button_bindings[5]  = (struct button_binding){KEY_Y,         MOD_CTRL  };  // Ctrl+Y
    button_bindings[6]  = (struct button_binding){KEY_MINUS,     MOD_CTRL  };  // Ctrl+-
    button_bindings[7]  = (struct button_binding){KEY_EQUAL,     MOD_CTRL  };  // Ctrl+=  (zoom in)
    button_bindings[8]  = (struct button_binding){KEY_VOLUMEUP,  0         };  // Volume Up
    button_bindings[9]  = (struct button_binding){0,             0         };  // Blank for Quadrant Mode

    return 0;
}

static void press_binding(struct button_binding *b, struct input_dev *button_input_dev) {
    if (!button_input_dev || b->keycode == 0)
        return;

    // press modifiers
    if (b->modifiers & MOD_CTRL)
        input_report_key(button_input_dev, KEY_LEFTCTRL, 1);
    if (b->modifiers & MOD_ALT)
        input_report_key(button_input_dev, KEY_LEFTALT, 1);
    if (b->modifiers & MOD_SHIFT)
        input_report_key(button_input_dev, KEY_LEFTSHIFT, 1);

    // press and release the main key
    input_report_key(button_input_dev, b->keycode, 1);
}


static void release_binding(struct button_binding *b, struct input_dev *button_input_dev) {
    input_report_key(button_input_dev, b->keycode, 0);

    // release modifiers
    if (b->modifiers & MOD_CTRL)
        input_report_key(button_input_dev, KEY_LEFTCTRL, 0);
    if (b->modifiers & MOD_ALT)
        input_report_key(button_input_dev, KEY_LEFTALT, 0);
    if (b->modifiers & MOD_SHIFT)
        input_report_key(button_input_dev, KEY_LEFTSHIFT, 0);
}

void update_button_states(struct button_array *buttons_pressed, struct input_dev *button_input_dev) {

    for (int i = 0; i < MAX_BUTTONS; i++) {
        release_binding(&button_bindings[i], button_input_dev);
    }

    for (int i = 0; i < buttons_pressed->no_pressed; i++) {
        int btn = buttons_pressed->buttons[i] - 1;
        
        if (btn == 9) {
            quadrant_mode = !quadrant_mode;
            printk(KERN_ALERT "Quadrant mode %s", quadrant_mode ? "enabled" : "disabled");
        }

        press_binding(&button_bindings[buttons_pressed->buttons[i] - 1], button_input_dev);
        pr_alert("button %d",i+1);
    }

    input_sync(button_input_dev);
}


void cursor_control_reporting(struct tablet_usb_dev *dev, struct tablet_event tab_data, int pen_in_range) {

    // I'm not 100% sure of the bit layout of the pen input data, so I've hard coded these values for now.
    // The local variables will be moved to data_parasing.c and populated based on the data received from the tablet in the future.
    // For now I just have it hard coded for testing, modularity can come afterwards.
    // - Ollie

    if (pen_in_range) {
        printk(KERN_ALERT "Pen in range\n");
        input_report_abs(dev->pen_input_dev, ABS_X,        tab_data.x);
        input_report_abs(dev->pen_input_dev, ABS_Y,        tab_data.y);
        input_report_abs(dev->pen_input_dev, ABS_PRESSURE, tab_data.pressure);

        input_report_key(dev->pen_input_dev, BTN_TOUCH,   tab_data.pressure);
        input_report_key(dev->pen_input_dev, BTN_STYLUS,  tab_data.pen_button  == 1);
        input_report_key(dev->pen_input_dev, BTN_STYLUS2, tab_data.pen_button == 2);
        input_report_key(dev->pen_input_dev, BTN_TOOL_PEN, 1);
    } else {
        input_report_key(dev->pen_input_dev, BTN_TOOL_PEN, 0);
        input_report_key(dev->pen_input_dev, BTN_TOUCH,    0);
    }

    input_sync(dev->pen_input_dev);
}

void cursor_control_init(struct tablet_usb_dev *dev) {

    dev->pen_input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);

    dev->pen_input_dev->name = "Custom Tablet";
    usb_make_path(dev->usb_dev, dev->phys, sizeof(dev->phys));
    dev->pen_input_dev->phys = dev->phys;
    dev->pen_input_dev->dev.parent = &dev->interface->dev;

    // Absolute axes
    input_set_abs_params(dev->pen_input_dev, ABS_X,        0, TABLET_MAX_X,        4, 0);
    input_set_abs_params(dev->pen_input_dev, ABS_Y,        0, TABLET_MAX_Y,        4, 0);
    input_set_abs_params(dev->pen_input_dev, ABS_PRESSURE, 0, TABLET_MAX_PRESSURE, 0, 0);

    input_abs_set_res(dev->pen_input_dev, ABS_X, TABLET_RES_X);
    input_abs_set_res(dev->pen_input_dev, ABS_Y, TABLET_RES_Y);

    // Pen buttons and touch
    __set_bit(BTN_TOUCH,    dev->pen_input_dev->keybit);
    __set_bit(BTN_STYLUS,   dev->pen_input_dev->keybit);
    __set_bit(BTN_STYLUS2,  dev->pen_input_dev->keybit);
    __set_bit(BTN_TOOL_PEN, dev->pen_input_dev->keybit);

    input_set_drvdata(dev->pen_input_dev, dev);

}

void quadrant_mode_reporting(struct tablet_usb_dev *dev, struct tablet_event tab_data, int pen_in_range) {
    int pen_touching = tab_data.pen_touching;

    if (pen_in_range && pen_touching && !dev->pen_was_touching) {
        if (tab_data.x < (TABLET_MAX_X / 2)) {
            if (tab_data.y < (TABLET_MAX_Y / 2)) {
                printk(KERN_ALERT "Quadrant 1");
                press_binding(&button_bindings[8], dev->button_input_dev);
            } else {
                printk(KERN_ALERT "Quadrant 3");
                press_binding(&button_bindings[0], dev->button_input_dev);
            }
        } else {
            if (tab_data.y < (TABLET_MAX_Y / 2)) {
                printk(KERN_ALERT "Quadrant 2");
                press_binding(&button_bindings[7], dev->button_input_dev);
            } else {
                printk(KERN_ALERT "Quadrant 4");
                press_binding(&button_bindings[6], dev->button_input_dev);
            }
        }
        input_sync(dev->button_input_dev);
    }

    if (dev->pen_was_touching && !pen_touching) {
        release_binding(&button_bindings[0], dev->button_input_dev);
        release_binding(&button_bindings[8], dev->button_input_dev);
        release_binding(&button_bindings[7], dev->button_input_dev);
        release_binding(&button_bindings[6], dev->button_input_dev);
        input_sync(dev->button_input_dev);
    }

    dev->pen_was_touching = pen_touching;
}