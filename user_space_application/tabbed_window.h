//
// Created by david-bol12 on 3/14/26.
//

#ifndef DEVICEDRIVERAPP_TABBED_WINDOW_H
#define DEVICEDRIVERAPP_TABBED_WINDOW_H
#include "raylib.h"

#define MAX_TABS 8

typedef struct {
    void (*draw) (void* tab_data);
    void* tab_draw_data;
    const char *label;
} Tab;

typedef struct {
    const Tab *tabs[MAX_TABS];
    int count;
    int active;
    int tabWidth;
    int tabHeight;
    Rectangle contentArea;
} TabBar;

Tab InitTab(void (*draw) (void* tab_data), void* tab_draw_data, const char* label);
TabBar InitTabBar(int x, int y, int tabHeight, int contentWidth, int contentHeight);
void AddTab(TabBar *tb, const Tab *tab);
void UpdateTabBar(TabBar *tb);
void DrawTabBar(TabBar *tb);

#endif //DEVICEDRIVERAPP_TABBED_WINDOW_H