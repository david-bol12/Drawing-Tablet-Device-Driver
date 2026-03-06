//
// Created by david-bol12 on 3/6/26.
//

#ifndef DEVICEDRIVERPROJECT_DATA_PARSING_H
#define DEVICEDRIVERPROJECT_DATA_PARSING_H

struct button_array {
    short no_pressed;
    char* buttons;
};

void get_buttons_pressed(unsigned char* data, u32 length, struct button_array* location);

#endif //DEVICEDRIVERPROJECT_DATA_PARSING_H