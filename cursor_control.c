#include "cursor_control.h"

#define TABLET_RES_X 200
#define TABLET_RES_Y 200


//TODO: remove unnecessary buffer
void cursor_control_reporting(struct tablet_usb_dev *dev, unsigned char *buf, short x, short y, int pressure) {

    // I'm not 100% sure of the bit layout of the pen input data, so I've hard coded these values for now.
    // The local variables will be moved to data_parasing.c and populated based on the data received from the tablet in the future.
    // For now I just have it hard coded for testing, modularity can come afterwards.
    // - Ollie

    int pen_in_range  = 1;
    int pen_touching  = 0;

    int btn1          = 0;
    int btn2          = 0;

    if (pen_in_range) {
        printk(KERN_ALERT "Pen in range");
        input_report_abs(dev->input_dev, ABS_X,        x);
        input_report_abs(dev->input_dev, ABS_Y,        y);
        input_report_abs(dev->input_dev, ABS_PRESSURE, pressure);

        input_report_key(dev->input_dev, BTN_TOUCH,   pen_touching);
        input_report_key(dev->input_dev, BTN_STYLUS,  btn1);
        input_report_key(dev->input_dev, BTN_STYLUS2, btn2);
        input_report_key(dev->input_dev, BTN_TOOL_PEN, 1);
    } else {
        input_report_key(dev->input_dev, BTN_TOOL_PEN, 0);
        input_report_key(dev->input_dev, BTN_TOUCH,    0);
    } 

    input_sync(dev->input_dev);
}

void cursor_control_initialize(struct tablet_usb_dev *dev) {

    dev->input_dev->name = "Custom Tablet";
    usb_make_path(dev->usb_dev, dev->phys, sizeof(dev->phys));
    dev->input_dev->phys = dev->phys;
    dev->input_dev->dev.parent = &dev->interface->dev;

    // Absolute axes
    input_set_abs_params(dev->input_dev, ABS_X,        0, TABLET_MAX_X,        4, 0);
    input_set_abs_params(dev->input_dev, ABS_Y,        0, TABLET_MAX_Y,        4, 0);
    input_set_abs_params(dev->input_dev, ABS_PRESSURE, 0, TABLET_MAX_PRESSURE, 0, 0);

    input_abs_set_res(dev->input_dev, ABS_X, TABLET_RES_X);
    input_abs_set_res(dev->input_dev, ABS_Y, TABLET_RES_Y);

    // Pen buttons and touch
    __set_bit(BTN_TOUCH,    dev->input_dev->keybit);
    __set_bit(BTN_STYLUS,   dev->input_dev->keybit);
    __set_bit(BTN_STYLUS2,  dev->input_dev->keybit);
    __set_bit(BTN_TOOL_PEN, dev->input_dev->keybit);

    input_set_drvdata(dev->input_dev, dev);

}