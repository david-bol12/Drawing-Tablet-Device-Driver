//
// Created by david-bol12 on 3/14/26.
//

#include "tabbed_window.h"

#include <stddef.h>

#include "raylib.h"

Tab InitTab(void (*draw) (void* tab_data), void* tab_draw_data, const char* label) {
    Tab tab = {
        .draw = draw,
        .tab_draw_data = tab_draw_data,
        .label = label
    };

    return tab;
}


TabBar InitTabBar(int x, int y, int tabHeight, int contentWidth, int contentHeight)
{
    TabBar tb = { 0 };
    tb.tabWidth = contentWidth / MAX_TABS;
    tb.tabHeight = tabHeight;
    tb.contentArea = (Rectangle){ x, y + tabHeight, contentWidth, contentHeight };
    return tb;
}

void AddTab(TabBar *tb, const Tab *tab)
{
    if (tb->count < MAX_TABS)
        tb->tabs[tb->count++] = tab;
}

void UpdateTabBar(TabBar *tb)
{
    for (int i = 0; i < tb->count; i++)
    {
        Rectangle r = { tb->contentArea.x + i * tb->tabWidth,
                        tb->contentArea.y - tb->tabHeight,
                        tb->tabWidth, tb->tabHeight };
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(GetMousePosition(), r))
            tb->active = i;
    }
}

void DrawTabBar(TabBar *tb)
{
    // Draw tabs
    for (int i = 0; i < tb->count; i++)
    {
        Rectangle r = { tb->contentArea.x + i * tb->tabWidth,
                        tb->contentArea.y - tb->tabHeight,
                        tb->tabWidth, tb->tabHeight };
        bool isActive = (i == tb->active);
        DrawRectangleRec(r, isActive ? SKYBLUE : LIGHTGRAY);
        DrawRectangleLinesEx(r, 2, isActive ? BLUE : GRAY);
        DrawText(tb->tabs[i]->label,
            r.x + tb->tabWidth / 2 - MeasureText(tb->tabs[i]->label, 20) / 2,
            r.y + tb->tabHeight / 2 - 10,
            20, isActive ? WHITE : DARKGRAY);
    }

    // Draw content area border
    DrawRectangleLinesEx(tb->contentArea, 2, GRAY);

    // Draw Tab content
    tb->tabs[tb->active]->draw(
        tb->tabs[tb->active]->tab_draw_data
        );
}