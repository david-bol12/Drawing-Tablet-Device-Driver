//
// Created by david-bol12 on 3/6/26.
//

#ifndef DEVICEDRIVERPROJECT_DATA_PARSING_H
#define DEVICEDRIVERPROJECT_DATA_PARSING_H

#include "tablet.h"

void get_buttons_pressed(unsigned char* data, unsigned int length, struct button_array* location);

void update_pen_data(unsigned char* buf, unsigned int length, struct tablet_event *tablet_data);

#endif //DEVICEDRIVERPROJECT_DATA_PARSING_H