//
// Created by david-bol12 on 3/14/26.
//

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "raylib.h"
#include "../tablet.h"

// Basic AI gen code to convert raylib key num to key name

const char *RayKeyToString(int key, bool shift)
{
    if (key >= KEY_A && key <= KEY_Z)
    {
        static char letter[2] = {0};
        letter[0] = (char)key;
        return letter;
    }

    if (key >= KEY_ZERO && key <= KEY_NINE)
    {
        if (shift)
        {
            const char *shifted = "!@#$%^&*()";
            static char sym[2] = {0};
            sym[0] = shifted[key - KEY_ZERO];
            return sym;
        }
        static char num[2] = {0};
        num[0] = (char)key;
        return num;
    }

    if (key >= KEY_KP_0 && key <= KEY_KP_9)
        return TextFormat("KP%d", key - KEY_KP_0);

    if (key >= KEY_F1 && key <= KEY_F12)
        return TextFormat("F%d", key - KEY_F1 + 1);

    switch (key)
    {
        case KEY_SPACE:         return " ";
        case KEY_ENTER:         return "ENTER";
        case KEY_TAB:           return "TAB";
        case KEY_BACKSPACE:     return "BACKSPACE";
        case KEY_ESCAPE:        return "ESCAPE";
        case KEY_DELETE:        return "DELETE";
        case KEY_INSERT:        return "INSERT";
        case KEY_LEFT_SHIFT:    return "LEFT_SHIFT";
        case KEY_RIGHT_SHIFT:   return "RIGHT_SHIFT";
        case KEY_LEFT_CONTROL:  return "LEFT_CTRL";
        case KEY_RIGHT_CONTROL: return "RIGHT_CTRL";
        case KEY_LEFT_ALT:      return "LEFT_ALT";
        case KEY_RIGHT_ALT:     return "RIGHT_ALT";
        case KEY_LEFT_SUPER:    return "LEFT_SUPER";
        case KEY_RIGHT_SUPER:   return "RIGHT_SUPER";
        case KEY_UP:            return "UP";
        case KEY_DOWN:          return "DOWN";
        case KEY_LEFT:          return "LEFT";
        case KEY_RIGHT:         return "RIGHT";
        case KEY_HOME:          return "HOME";
        case KEY_END:           return "END";
        case KEY_PAGE_UP:       return "PAGE_UP";
        case KEY_PAGE_DOWN:     return "PAGE_DOWN";
        case KEY_MINUS:         return shift ? "_" : "-";
        case KEY_EQUAL:         return shift ? "+" : "=";
        case KEY_LEFT_BRACKET:  return shift ? "{" : "[";
        case KEY_RIGHT_BRACKET: return shift ? "}" : "]";
        case KEY_BACKSLASH:     return shift ? "|" : "\\";
        case KEY_SEMICOLON:     return shift ? ":" : ";";
        case KEY_APOSTROPHE:    return shift ? "\"" : "'";
        case KEY_COMMA:         return shift ? "<" : ",";
        case KEY_PERIOD:        return shift ? ">" : ".";
        case KEY_SLASH:         return shift ? "?" : "/";
        case KEY_GRAVE:         return shift ? "~" : "`";
        case KEY_KP_DECIMAL:    return ".";
        case KEY_KP_DIVIDE:     return "/";
        case KEY_KP_MULTIPLY:   return "*";
        case KEY_KP_SUBTRACT:   return "-";
        case KEY_KP_ADD:        return "+";
        case KEY_KP_ENTER:      return "ENTER";
        case KEY_KP_EQUAL:      return "=";
        case KEY_CAPS_LOCK:     return "CAPS_LOCK";
        case KEY_SCROLL_LOCK:   return "SCROLL_LOCK";
        case KEY_NUM_LOCK:      return "NUM_LOCK";
        case KEY_PRINT_SCREEN:  return "PRINT_SCREEN";
        case KEY_PAUSE:         return "PAUSE";
        case KEY_MENU:          return "MENU";
        default:                return "UNKNOWN";
    }
}

int getModifiers() {

    int modifiers = 0;

    bool ctrl  = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool shift = IsKeyDown(KEY_LEFT_SHIFT)   || IsKeyDown(KEY_RIGHT_SHIFT);
    bool alt   = IsKeyDown(KEY_LEFT_ALT)     || IsKeyDown(KEY_RIGHT_ALT);

    if (shift) {
        modifiers += 4;
    }
    if (alt) {
        modifiers += 2;
    }
    if (ctrl) {
        modifiers += 1;
    }

    return modifiers;
}

const char *GetKeyCombo(int key, int modifiers)
{

    if (key == 0) return NULL;

    bool ctrl  = modifiers & MOD_CTRL;
    bool shift = modifiers & MOD_SHIFT;
    bool alt   = modifiers & MOD_ALT;

    // Build modifier prefix
    static char combo[64] = {0};
    combo[0] = '\0';


    if (ctrl)  strcat(combo, "CTRL+");
    if (shift) strcat(combo, "SHIFT+");
    if (alt)   strcat(combo, "ALT+");

    // Don't report if the key itself is a modifier
    if (key == KEY_LEFT_SHIFT   || key == KEY_RIGHT_SHIFT   ||
        key == KEY_LEFT_CONTROL || key == KEY_RIGHT_CONTROL ||
        key == KEY_LEFT_ALT     || key == KEY_RIGHT_ALT     ||
        key == KEY_LEFT_SUPER   || key == KEY_RIGHT_SUPER)
        return combo;

    const char *keyName = RayKeyToString(key, shift);

    strcat(combo, keyName);

    return combo;
}

// AI gen code to convert raylib key values to kernel key values + vice versa

int RaylibToLinuxKey(int raylibKey)
{
    switch (raylibKey)
    {
        // Letters
        case 65:  return 30;  // A
        case 66:  return 48;  // B
        case 67:  return 46;  // C
        case 68:  return 32;  // D
        case 69:  return 18;  // E
        case 70:  return 33;  // F
        case 71:  return 34;  // G
        case 72:  return 35;  // H
        case 73:  return 23;  // I
        case 74:  return 36;  // J
        case 75:  return 37;  // K
        case 76:  return 38;  // L
        case 77:  return 50;  // M
        case 78:  return 49;  // N
        case 79:  return 24;  // O
        case 80:  return 25;  // P
        case 81:  return 16;  // Q
        case 82:  return 19;  // R
        case 83:  return 31;  // S
        case 84:  return 20;  // T
        case 85:  return 22;  // U
        case 86:  return 47;  // V
        case 87:  return 17;  // W
        case 88:  return 45;  // X
        case 89:  return 21;  // Y
        case 90:  return 44;  // Z

        // Numbers
        case 48:  return 11;  // 0
        case 49:  return 2;   // 1
        case 50:  return 3;   // 2
        case 51:  return 4;   // 3
        case 52:  return 5;   // 4
        case 53:  return 6;   // 5
        case 54:  return 7;   // 6
        case 55:  return 8;   // 7
        case 56:  return 9;   // 8
        case 57:  return 10;  // 9

        // Function keys
        case 290: return 59;  // F1
        case 291: return 60;  // F2
        case 292: return 61;  // F3
        case 293: return 62;  // F4
        case 294: return 63;  // F5
        case 295: return 64;  // F6
        case 296: return 65;  // F7
        case 297: return 66;  // F8
        case 298: return 67;  // F9
        case 299: return 68;  // F10
        case 300: return 87;  // F11
        case 301: return 88;  // F12

        // Modifiers
        case 340: return 42;  // LEFT_SHIFT
        case 344: return 54;  // RIGHT_SHIFT
        case 341: return 29;  // LEFT_CTRL
        case 345: return 97;  // RIGHT_CTRL
        case 342: return 56;  // LEFT_ALT
        case 346: return 100; // RIGHT_ALT
        case 343: return 125; // LEFT_SUPER
        case 347: return 126; // RIGHT_SUPER

        // Navigation
        case 265: return 103; // UP
        case 264: return 108; // DOWN
        case 263: return 105; // LEFT
        case 262: return 106; // RIGHT
        case 268: return 102; // HOME
        case 269: return 107; // END
        case 266: return 104; // PAGE_UP
        case 267: return 109; // PAGE_DOWN
        case 260: return 110; // INSERT
        case 261: return 111; // DELETE

        // Whitespace / control
        case 32:  return 57;  // SPACE
        case 257: return 28;  // ENTER
        case 258: return 15;  // TAB
        case 259: return 14;  // BACKSPACE
        case 256: return 1;   // ESCAPE

        // Punctuation
        case 45:  return 12;  // MINUS
        case 61:  return 13;  // EQUAL
        case 91:  return 26;  // LEFT_BRACKET
        case 93:  return 27;  // RIGHT_BRACKET
        case 92:  return 43;  // BACKSLASH
        case 59:  return 39;  // SEMICOLON
        case 39:  return 40;  // APOSTROPHE
        case 44:  return 51;  // COMMA
        case 46:  return 52;  // PERIOD
        case 47:  return 53;  // SLASH
        case 96:  return 41;  // GRAVE

        // Numpad
        case 320: return 82;  // KP_0
        case 321: return 79;  // KP_1
        case 322: return 80;  // KP_2
        case 323: return 81;  // KP_3
        case 324: return 75;  // KP_4
        case 325: return 76;  // KP_5
        case 326: return 77;  // KP_6
        case 327: return 71;  // KP_7
        case 328: return 72;  // KP_8
        case 329: return 73;  // KP_9
        case 330: return 83;  // KP_DECIMAL
        case 331: return 98;  // KP_DIVIDE
        case 332: return 55;  // KP_MULTIPLY
        case 333: return 74;  // KP_SUBTRACT
        case 334: return 78;  // KP_ADD
        case 335: return 96;  // KP_ENTER
        case 336: return 117; // KP_EQUAL

        // Misc
        case 280: return 58;  // CAPS_LOCK
        case 281: return 70;  // SCROLL_LOCK
        case 282: return 69;  // NUM_LOCK
        case 283: return 99;  // PRINT_SCREEN
        case 284: return 119; // PAUSE
        case 348: return 127; // MENU

        default:  return -1;  // unknown
    }
}

int LinuxKeyToRaylib(int linuxKey)
{
    switch (linuxKey)
    {
        // Letters
        case 30:  return 65;  // A
        case 48:  return 66;  // B
        case 46:  return 67;  // C
        case 32:  return 68;  // D
        case 18:  return 69;  // E
        case 33:  return 70;  // F
        case 34:  return 71;  // G
        case 35:  return 72;  // H
        case 23:  return 73;  // I
        case 36:  return 74;  // J
        case 37:  return 75;  // K
        case 38:  return 76;  // L
        case 50:  return 77;  // M
        case 49:  return 78;  // N
        case 24:  return 79;  // O
        case 25:  return 80;  // P
        case 16:  return 81;  // Q
        case 19:  return 82;  // R
        case 31:  return 83;  // S
        case 20:  return 84;  // T
        case 22:  return 85;  // U
        case 47:  return 86;  // V
        case 17:  return 87;  // W
        case 45:  return 88;  // X
        case 21:  return 89;  // Y
        case 44:  return 90;  // Z

        // Numbers
        case 11:  return 48;  // 0
        case 2:   return 49;  // 1
        case 3:   return 50;  // 2
        case 4:   return 51;  // 3
        case 5:   return 52;  // 4
        case 6:   return 53;  // 5
        case 7:   return 54;  // 6
        case 8:   return 55;  // 7
        case 9:   return 56;  // 8
        case 10:  return 57;  // 9

        // Function keys
        case 59:  return 290; // F1
        case 60:  return 291; // F2
        case 61:  return 292; // F3
        case 62:  return 293; // F4
        case 63:  return 294; // F5
        case 64:  return 295; // F6
        case 65:  return 296; // F7
        case 66:  return 297; // F8
        case 67:  return 298; // F9
        case 68:  return 299; // F10
        case 87:  return 300; // F11
        case 88:  return 301; // F12

        // Modifiers
        case 42:  return 340; // LEFT_SHIFT
        case 54:  return 344; // RIGHT_SHIFT
        case 29:  return 341; // LEFT_CTRL
        case 97:  return 345; // RIGHT_CTRL
        case 56:  return 342; // LEFT_ALT
        case 100: return 346; // RIGHT_ALT
        case 125: return 343; // LEFT_SUPER
        case 126: return 347; // RIGHT_SUPER

        // Navigation
        case 103: return 265; // UP
        case 108: return 264; // DOWN
        case 105: return 263; // LEFT
        case 106: return 262; // RIGHT
        case 102: return 268; // HOME
        case 107: return 269; // END
        case 104: return 266; // PAGE_UP
        case 109: return 267; // PAGE_DOWN
        case 110: return 260; // INSERT
        case 111: return 261; // DELETE

        // Whitespace / control
        case 57:  return 32;  // SPACE
        case 28:  return 257; // ENTER
        case 15:  return 258; // TAB
        case 14:  return 259; // BACKSPACE
        case 1:   return 256; // ESCAPE

        // Punctuation
        case 12:  return 45;  // MINUS
        case 13:  return 61;  // EQUAL
        case 26:  return 91;  // LEFT_BRACKET
        case 27:  return 93;  // RIGHT_BRACKET
        case 43:  return 92;  // BACKSLASH
        case 39:  return 59;  // SEMICOLON
        case 40:  return 39;  // APOSTROPHE
        case 51:  return 44;  // COMMA
        case 52:  return 46;  // PERIOD
        case 53:  return 47;  // SLASH
        case 41:  return 96;  // GRAVE

        // Numpad
        case 82:  return 320; // KP_0
        case 79:  return 321; // KP_1
        case 80:  return 322; // KP_2
        case 81:  return 323; // KP_3
        case 75:  return 324; // KP_4
        case 76:  return 325; // KP_5
        case 77:  return 326; // KP_6
        case 71:  return 327; // KP_7
        case 72:  return 328; // KP_8
        case 73:  return 329; // KP_9
        case 83:  return 330; // KP_DECIMAL
        case 98:  return 331; // KP_DIVIDE
        case 55:  return 332; // KP_MULTIPLY
        case 74:  return 333; // KP_SUBTRACT
        case 78:  return 334; // KP_ADD
        case 96:  return 335; // KP_ENTER
        case 117: return 336; // KP_EQUAL

        // Misc
        case 58:  return 280; // CAPS_LOCK
        case 70:  return 281; // SCROLL_LOCK
        case 69:  return 282; // NUM_LOCK
        case 99:  return 283; // PRINT_SCREEN
        case 119: return 284; // PAUSE
        case 127: return 348; // MENU

        default:  return -1;  // unknown
    }
}