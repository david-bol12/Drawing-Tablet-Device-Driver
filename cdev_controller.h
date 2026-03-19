//
// Created by david-bol12 on 3/7/26.
//

#ifndef DEVICEDRIVERPROJECT_CDEV_CONTROLLER_H
#define DEVICEDRIVERPROJECT_CDEV_CONTROLLER_H

int tablet_cdev_init(void);
void tablet_cdev_cleanup(void);
int cdev_buffer_write(struct tablet_event *event);
int cdev_buffer_read(struct tablet_event *event);


#endif //DEVICEDRIVERPROJECT_CDEV_CONTROLLER_H
