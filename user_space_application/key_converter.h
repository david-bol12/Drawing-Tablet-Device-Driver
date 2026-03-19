//
// Created by david-bol12 on 3/14/26.
//

#ifndef DEVICEDRIVERAPP_KEY_CONVERTER_H
#define DEVICEDRIVERAPP_KEY_CONVERTER_H
#include <stdbool.h>

const char *RayKeyToString(int key, bool shift);
const char *GetKeyCombo(int key, int modifiers);
int LinuxKeyToRaylib(int linuxKey);

#endif //DEVICEDRIVERAPP_KEY_CONVERTER_H