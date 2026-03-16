//
// Created by david-bol12 on 3/6/26.
//
#include <linux/init.h>
#include <linux/usb.h>
#include "data_parsing.h"

MODULE_LICENSE("Dual BSD/GPL");

char get_button_val(unsigned char code) {
    switch (code) {
        case 0x05: return 1;
        case 0x08: return 2;
        case 0x04: return 3;
        case 0x2C: return 4;
        case 0x19: return 5;
        case 0x16: return 6;
        case 0x1D: return 7;
        case 0x11: return 8;
        case 0x57: return 9;
        case 0x56: return 10;
        default: return 0; // not found
    }
}

void get_buttons_pressed(unsigned char* data, unsigned int length, struct button_array* location) {
    int index = 0;
    char val;
    struct button_array* buttons = location;

    buttons->no_pressed = 0;

    val = get_button_val(data[1]);
    if (val != 0 && val != 1) { // 8 causes the first byte to be 05 which is the code for button 1
        buttons->buttons[index] = val;
        index++;
        buttons->no_pressed++;
    }

    for (int i = 2; i < length; i++) {
        val = get_button_val(data[i]);
        if (val != 0) {
            buttons->buttons[index] = val;
            index++;
            buttons->no_pressed++;
        }
    }
}

struct point get_pen_coordinates(unsigned char* data, unsigned int length) {
    unsigned short x = 0;
    unsigned short y = 0;

    x = data[3] << 8;
    x += data[2];

    y = data[5] << 8;
    y += data[4];

    struct point point = {
        x,
        y
    };

    return point;
}
