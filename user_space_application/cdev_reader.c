//
// Created by david-bol12 on 3/14/26.
//

#include "cdev_reader.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

int init_reader() {
    int fd = open("/dev/Tablet Character Device", O_RDONLY);
    return fd;
}

void get_tablet_event(int fd, struct tablet_event* event_buf) {
    read(fd, event_buf, sizeof(struct tablet_event));
}

void do_ioctl(int fd) {
    struct button_binding binding = {
        1,
        115,
        0
    };
    ioctl(fd, TABLET_SET_BINDING, &binding);
    printf("\n ioctl done \n");
}

void get_settings(int fd, struct tablet_settings *tablet_settings) {
    ioctl(fd, TABLET_GET_SETTING, tablet_settings);
    printf("ran");
    printf("\n Keycode: %d \n", tablet_settings->tab_bindings[0].keycode);
}

void* cdev_read(void* reader_args) {
    struct reader_args *args = reader_args;
    int fd = args->fd;
    while (1) {
        get_tablet_event(fd, args->event_buf);
    }
}