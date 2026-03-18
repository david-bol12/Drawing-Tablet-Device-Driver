//
// Created by david-bol12 on 3/13/26.
//

#ifndef DEVICEDRIVERPROJECT_INPUT_EVENTS_H
#define DEVICEDRIVERPROJECT_INPUT_EVENTS_H

#include <linux/input.h>
#include <linux/types.h>

#include "usb_driver.h"
#include "tablet.h"

void cursor_control_reporting(struct tablet_usb_dev *dev, struct tablet_event tab_data, int pen_in_range);
void cursor_control_init(struct tablet_usb_dev *dev);
int button_dev_init(struct input_dev *button_input_dev);
void update_button_states(struct button_array *buttons_pressed, struct input_dev *button_input_dev);

#endif //DEVICEDRIVERPROJECT_INPUT_EVENTS_H