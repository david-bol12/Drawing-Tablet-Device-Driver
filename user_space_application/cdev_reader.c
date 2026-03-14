//
// Created by david-bol12 on 3/14/26.
//

#include "cdev_reader.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

int init_reader() {
    int fd = open("/dev/Tablet Character Device", O_RDONLY);
    return fd;
}

void get_tablet_event(int fd, struct tablet_event* event_buf) {
    read(fd, event_buf, sizeof(struct tablet_event));
}

void* cdev_read(void* event_buf) {
    int fd = init_reader();
    while (1) {
        get_tablet_event(fd, event_buf);
    }
}