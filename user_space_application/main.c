//
// Created by david-bol12 on 3/13/26.
//

#include <stdio.h>

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "tabbed_window.h"
#include "src/raygui.h"
#include "key_converter.h"
#include <stdbool.h>
#include <pthread.h>

#include "cdev_reader.h"
#include "../tablet.h"

#define MAIN_FONTSIZE 20

void draw1 (void) {

}

void draw2 (void* tab_data) {
    DrawText(TextFormat("On tab 2"), 200, 200, 30, DARKGRAY);
}

void drawTabletStats(void* event_buf) {

    //TODO: Add rest of tablet stats

    struct tablet_event *tab_event = event_buf;
    DrawText(TextFormat("X Coordinates: %d", tab_event->x), 80, 110, MAIN_FONTSIZE, BLACK);
    DrawText(TextFormat("Y Coordinates: %d", tab_event->y), 80, 130, MAIN_FONTSIZE, BLACK);
    DrawText(TextFormat("Pen Pressure: %d", tab_event->pressure), 80, 150, MAIN_FONTSIZE, BLACK);
}

void drawTabBindingMenu(int x, int y, int button_no) {
    DrawText(TextFormat("Tablet Button %d:", button_no), x, y, MAIN_FONTSIZE, BLACK);
}

int main(void)
{
    const int screenWidth = 1500;
    const int screenHeight = 800;

    struct tablet_event *event_buf = malloc(sizeof(struct tablet_event));

    pthread_t cdev_reader;
    pthread_create(&cdev_reader, NULL, cdev_read, event_buf);

    InitWindow(screenWidth, screenHeight, "Raylib Keyboard Input");
    SetWindowState(FLAG_WINDOW_MAXIMIZED);
    SetTargetFPS(60);

    TabBar tb = InitTabBar(20, 55, 35, screenWidth - 50, screenHeight - 100);
    Tab tabs[2];
    tabs[0] = InitTab(drawTabletStats, event_buf, "Tablet Stats");
    tabs[1] = InitTab(draw2, NULL, "Button Binding");

    for (int i = 0; i < 2; i++) {
        AddTab(&tb, &tabs[i]);
    }



    Rectangle button = { 300, 200, 200, 50 };
    char combo[64] = "None";

    bool dropdownOpen = false;
    int selectedItem = 0;
    const char *options = "Option 1;Option 2;Option 3;Option 4";

    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);


    while (!WindowShouldClose())
    {

        UpdateTabBar(&tb);

        // drawTabBindingMenu(80, 120, 1);

        const char* str = NULL;

        int key = GetKeyPressed();
        if (key != 0)
        {
            str = GetKeyCombo(key);
        }


        if (str != NULL) {
            TextCopy(combo, TextFormat("%s", str));
        }

        // if (GuiDropdownBox((Rectangle){ 250, 160, 300, 40 }, options, &selectedItem, dropdownOpen))
        //     dropdownOpen = !dropdownOpen;


        BeginDrawing();
        DrawTabBar(&tb);
        ClearBackground(RAYWHITE);
        // DrawText(TextFormat("Keys: %d", event_buf->x), 100, 200, 30, DARKGRAY);
        EndDrawing();
    }


    free(event_buf);
    CloseWindow();
    return 0;
}