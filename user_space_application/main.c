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
#include <sys/ioctl.h>

#include "cdev_reader.h"
#include "../tablet.h"

#define MAIN_FONTSIZE 25

struct tablet_settings *tablet_settings;

void draw1 (void) {

}

void drawTabBindingMenu(int x, int y, struct button_binding binding) {



    const static char* str = "None";
    static char combo[64] = "None";
    int key = LinuxKeyToRaylib(binding.keycode);
    if (key != 0)
    {
        str = GetKeyCombo(key, binding.modifiers);
    }


    if (str != NULL) {
        TextCopy(combo, TextFormat("%s", str));
    }
    DrawText(TextFormat("Tablet Button %d: %s", binding.button_id, str), x, y, MAIN_FONTSIZE, BLACK);
    if (GuiButton((Rectangle) {500, y, 100, 25}, "Click")) {
        printf("CLicked me! \n");
        fflush(stdout); 
    }
}

void draw2 () {
    int y = 110;
    for (int i = 0; i < MAX_BUTTONS; i++) {
        drawTabBindingMenu(80, y, tablet_settings->tab_bindings[i]);
        y += 30;
    }
}



void drawTabletStats(void* event_buf) {

    //TODO: Add rest of tablet stats

    struct tablet_event *tab_event = event_buf;

    Color pen_in_range_colour;
    static const char* pen_in_range_message;

    if (tab_event->pen_in_range) {
        pen_in_range_colour = GREEN;
        pen_in_range_message = "Pen in Range";
    } else {
        pen_in_range_colour = RED;
        pen_in_range_message = "Pen out of Range";
    }

    char tab_buttons_pressed[30] = {0};

    if (tab_event->tab_buttons.no_pressed == 0) {
        strcat(tab_buttons_pressed, "None");
    } else {
        for (int i = 0; i < tab_event->tab_buttons.no_pressed; i++) {
            strcat(tab_buttons_pressed, TextFormat("%d, ", tab_event->tab_buttons.buttons[i]));
        }
    }

    DrawText(pen_in_range_message, 75, 110, MAIN_FONTSIZE, pen_in_range_colour);

    DrawText(TextFormat("X Coordinates: %d \nY Coordinates: %d \nPen Pressure: %d \nPen Button Pressed: %d \nTablet Buttons Pressed: %s",
        tab_event->x, tab_event->y, tab_event->pressure, tab_event->pen_button, tab_buttons_pressed),
        80, 140, MAIN_FONTSIZE, BLACK);

}


int main(void)
{
    const int screenWidth = 1500;
    const int screenHeight = 800;

    struct tablet_event *event_buf = malloc(sizeof(struct tablet_event));
    tablet_settings = malloc(sizeof(struct tablet_settings));

    int fd = init_reader();

    struct reader_args reader_args = {
        event_buf,
        tablet_settings,
        fd
    };

    get_settings(fd, tablet_settings);

    pthread_t cdev_reader;
    pthread_create(&cdev_reader, NULL, cdev_read, &reader_args);

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

    bool dropdownOpen = false;
    int selectedItem = 0;
    const char *options = "Option 1;Option 2;Option 3;Option 4";

    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);


    while (!WindowShouldClose())
    {


        // drawTabBindingMenu(80, 120, 1);




        // if (GuiDropdownBox((Rectangle){ 250, 160, 300, 40 }, options, &selectedItem, dropdownOpen))
        //     dropdownOpen = !dropdownOpen;


        BeginDrawing();
        UpdateTabBar(&tb);
        DrawTabBar(&tb);
        ClearBackground(RAYWHITE);
        // DrawText(TextFormat("Keys: %d", event_buf->x), 100, 200, 30, DARKGRAY);
        EndDrawing();
    }


    free(event_buf);
    CloseWindow();
    return 0;
}