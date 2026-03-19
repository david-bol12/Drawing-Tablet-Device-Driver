#ifndef IOCTL_H
#define IOCTL_H

#include <linux/fs.h>
#include <linux/mutex.h>
#include "tablet.h"

#define MAX_BUTTONS 11

extern struct button_binding button_bindings[MAX_BUTTONS];
extern struct mutex bindings_mutex;



#endif
