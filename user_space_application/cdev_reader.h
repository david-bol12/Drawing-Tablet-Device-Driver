//
// Created by david-bol12 on 3/14/26.
//

#ifndef DEVICEDRIVERAPP_CDEV_READER_H
#define DEVICEDRIVERAPP_CDEV_READER_H

#include "../tablet.h"

struct reader_args {
    struct tablet_event *event_buf;
    struct tablet_settings *tablet_settings;
    int fd;
};

void* cdev_read(void* event_buf);
int init_reader();
void get_settings(int fd, struct tablet_settings *tablet_settings);

#endif //DEVICEDRIVERAPP_CDEV_READER_H