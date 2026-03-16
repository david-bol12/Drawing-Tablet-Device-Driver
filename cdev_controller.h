//
// Created by david-bol12 on 3/7/26.
//

#ifndef DEVICEDRIVERPROJECT_CDEV_CONTROLLER_H
#define DEVICEDRIVERPROJECT_CDEV_CONTROLLER_H

int tablet_cdev_init(void);
void tablet_cdev_cleanup(void);
static int cdev_open(struct inode *inode, struct file *file);
static int cdev_release(struct inode *inode, struct file *file);
static ssize_t cdev_read(struct file *file, char __user *user_buf, size_t count, loff_t *offset);
static ssize_t cdev_write(struct file *file, const char __user *user_buf, size_t count, loff_t *offset);
void cdev_buffer_clear(void);
int cdev_buffer_write(struct tablet_event *event);
int cdev_buffer_read(struct tablet_event *event);


#endif //DEVICEDRIVERPROJECT_CDEV_CONTROLLER_H