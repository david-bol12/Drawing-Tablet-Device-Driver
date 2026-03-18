#ifndef TABLET_H
#define TABLET_H

#define MAX_BUTTONS 10
#define BUTTON_INTERFACE 0
#define PEN_INTERFACE 1

struct button_array {
    short no_pressed;
    char buttons[7];
};

struct tablet_event {
    int x;
    int y;
    int pressure;
    struct button_array tab_buttons;     // button number 1-10, 0 = no button
    int pen_button;
    int pen_in_range;
};

struct button_binding {
    int keycode;    // Linux keycode e.g. KEY_Z
    int modifiers;  // bitmask: 1=Ctrl, 2=Alt, 4=Shift
};

struct tablet_settings {
    int maxX;
    int maxY;
    struct button_binding tab_bindings[MAX_BUTTONS];
    int toggle_bindings;
};

// unique identifier character
#define TABLET_MAGIC      'T'
// read from driver
#define TABLET_GET_EVENT  _IOR(TABLET_MAGIC, 1, struct tablet_event)
// no data transfer / a signal
#define TABLET_CLR_BUFFER _IO(TABLET_MAGIC,  2)
// sends a button_binding struct to driver to register new mapping (FOR GUI)
#define TABLET_SET_BINDING  _IOW(TABLET_MAGIC, 3, struct button_binding)
// user(GUI) asks driver what a button is currently mapped to
#define TABLET_GET_BINDING  _IOR(TABLET_MAGIC, 4, struct button_binding)
// user(GUI) tells driver to wipe all bindings
#define TABLET_CLR_BINDINGS _IO(TABLET_MAGIC,  5)
// inject a button event directly (for testing without hardware)
#define TABLET_INJECT_EVENT _IOW(TABLET_MAGIC, 6, struct tablet_event)

// modifier bitmask values
#define MOD_CTRL  1
#define MOD_ALT   2
#define MOD_SHIFT 4

#endif
