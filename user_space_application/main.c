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

struct binding_menu {
    struct button_binding *current_binding;
    bool edit_mode;
    struct button_binding latched_binding;
    char combo[64];
    bool dropdownOpen;
    int selectedItem;
};

struct tablet_settings *tablet_settings;
struct binding_menu binding_menus[MAX_BUTTONS];
int fd;
const int linux_keycodes[] = {-1, 115, 114, 164, 113, 224, 225};

void draw1 (void) {

}

void initBindingMenu() {
    for (int i = 0; i < MAX_BUTTONS; i++) {
        struct binding_menu binding_menu = {
            .current_binding = &tablet_settings->tab_bindings[i],
            .edit_mode = false,
            .latched_binding = tablet_settings->tab_bindings[i],
            .combo = "None",
            .dropdownOpen = false,
            .selectedItem = 0
        };

        binding_menus[i] = binding_menu;
    }
}

void drawTabBindingMenu(int x, int y, struct binding_menu *menu_info, int pressed_key) {

    char* button_text;
    bool custom_input = menu_info->selectedItem == 0 ? true : false;

    const char *options = "Custom Input;Volume Up;Volume Down;Play/Pause;Mute;Brightness Up;Brightness Down";

    static bool dropDownOpen = false;

    if (!custom_input) {
        button_text = "Update";
    } else if (menu_info->edit_mode) {
        button_text = "Save";
    } else {
        button_text = "Edit";
    }

    const char* str = NULL;
    int key;

    if (menu_info->edit_mode) {
        key = pressed_key;
        int modifiers = getModifiers();
        if (key != 0)
        {
            struct button_binding binding = {
                .modifiers = modifiers,
                .button_id = menu_info->current_binding->button_id,
                .keycode = RaylibToLinuxKey(key)
            };
            str = GetKeyCombo(key, modifiers);
            menu_info->latched_binding = binding;
        }
    } else {
        key = LinuxKeyToRaylib(menu_info->current_binding->keycode);
        if (key != 0)
            str = GetKeyCombo(key, menu_info->current_binding->modifiers);
    }

    bool dropBoxVal = GuiDropdownBox((Rectangle){ 500, y, 300, 25 }, options, &menu_info->selectedItem, menu_info->dropdownOpen);

    if (dropBoxVal && (!dropDownOpen || menu_info->dropdownOpen)) {
        menu_info->dropdownOpen = !menu_info->dropdownOpen;
        dropDownOpen = menu_info->dropdownOpen;
    }

    if (str != NULL) {
        TextCopy(menu_info->combo, TextFormat("%s", str));
    }
    DrawText(TextFormat("Tablet Button %d: %s", menu_info->current_binding->button_id, menu_info->combo), x, y, MAIN_FONTSIZE, BLACK);
    if (GuiButton((Rectangle) {800, y, 100, 25}, button_text)) {
        if (!custom_input) {
            printf("ran here");
            fflush(stdout);
            menu_info->edit_mode = false;
            struct button_binding binding = {
                menu_info->current_binding->button_id,
                linux_keycodes[menu_info->selectedItem],
                0
            };
            if (set_binding(fd, &binding) != -1) {
                tablet_settings->tab_bindings[menu_info->current_binding->button_id -1] = binding;
            }
            printf("ran here");
            fflush(stdout);
        }
        else if (menu_info->edit_mode) {
            if (set_binding(fd, &menu_info->latched_binding) != -1) {
                tablet_settings->tab_bindings[menu_info->current_binding->button_id -1] = menu_info->latched_binding;
            }
            menu_info->edit_mode = !menu_info->edit_mode;
        } else {
            menu_info->edit_mode = !menu_info->edit_mode;
        }

    }
}

void draw2 () {
    int y = 380;
    int key = GetKeyPressed();
    for (int i = MAX_BUTTONS -1; i >= 0; i--) {
        drawTabBindingMenu(80, y, &binding_menus[i], key);
        y -= 30;
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

    fd = init_reader();

    struct reader_args reader_args = {
        event_buf,
        tablet_settings,
        fd
    };

    get_settings(fd, tablet_settings);

    initBindingMenu();

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

    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);


    while (!WindowShouldClose())
    {
        BeginDrawing();
        UpdateTabBar(&tb);
        DrawTabBar(&tb);
        ClearBackground(RAYWHITE);
        EndDrawing();
    }


    free(event_buf);
    CloseWindow();
    return 0;
}