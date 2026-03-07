#ifndef TABLET_H
#define TABLET_H

#include "data_parsing.h"

struct tablet_event {
    int x;          
    int y;          
    int pressure;   
    struct button_array tab_buttons;
    int pen_button;
};

struct button_binding {
    int button_id;
    int keycode;
};

// unique identifier character
#define TABLET_MAGIC      'T'
// read from driver
#define TABLET_GET_EVENT  _IOR(TABLET_MAGIC, 1, struct tablet_event)
// no data transfer / a signal
#define TABLET_CLR_BUFFER _IO(TABLET_MAGIC,  2)
// sends a button_binding struct to driver to register new mapping (FOR GUI)
#define TABLET_SET_BINDING _IOW(TABLET_MAGIC, 3, struct button_binding)  // Don't know if this should be defined here
// user(GUI) asks driver what a button is currently mapped to
#define TABLET_GET_BINDING  _IOR(TABLET_MAGIC, 4, struct button_binding) // Same for this
// user(GUI) tells driver to wipe all bindings
#define TABLET_CLR_BINDINGS _IO(TABLET_MAGIC,  5)

#endif