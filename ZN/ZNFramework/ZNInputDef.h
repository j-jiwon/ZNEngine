#pragma once
#include "ZNFramework.h"

namespace ZNFramework
{
    enum class MOUSE_TYPE
    {
        LBUTTON = 0x01,
        RBUTTON = 0x02,
        MBUTTON = 0x03,
        UNKNOWN
    };

    enum class MOUSE_STATE
    {
        NONE,
        DOWN,
        MOVE,
        UP,
        END
    };

    enum class KEY_TYPE
    {
        // Arrow Keys
        KEY_LEFT = 0x25,       // Left Arrow key
        KEY_UP = 0x26,         // Up Arrow key
        KEY_RIGHT = 0x27,      // Right Arrow key
        KEY_DOWN = 0x28,       // Down Arrow key

        // Function Keys
        KEY_F1 = 0x70,           // F1 key
        KEY_F2 = 0x71,           // F2 key
        KEY_F3 = 0x72,           // F3 key
        KEY_F4 = 0x73,           // F4 key
        KEY_F5 = 0x74,           // F5 key
        KEY_F6 = 0x75,           // F6 key
        KEY_F7 = 0x76,           // F7 key
        KEY_F8 = 0x77,           // F8 key
        KEY_F9 = 0x78,           // F9 key
        KEY_F10 = 0x79,          // F10 key
        KEY_F11 = 0x7A,          // F11 key
        KEY_F12 = 0x7B,          // F12 key

        // Control Keys
        KEY_TAB = 0x09,         // Tab key
        KEY_SHIFT = 0x10,       // Shift key
        KEY_CONTROL = 0x11,     // Control key
        KEY_ALT = 0x12,         // Alt key
        KEY_CAPSLOCK = 0x14,    // Caps Lock key
        KEY_ESC = 0x1B,         // Escape key

        // Navigation Keys
        KEY_PAGEUP = 0x21,      // Page Up key
        KEY_PAGEDOWN = 0x22,    // Page Down key
        KEY_END = 0x23,         // End key
        KEY_HOME = 0x24,        // Home key

        // Insert/Delete Keys
        KEY_INSERT = 0x2D,   // Insert key
        KEY_DELETE = 0x2E,   // Delete key
        KEY_BACKSPACE = 0x08,  // Backspace key

        // Numeric Keys
        KEY_0 = 0x30,              // 0 key
        KEY_1 = 0x31,              // 1 key
        KEY_2 = 0x32,              // 2 key
        KEY_3 = 0x33,              // 3 key
        KEY_4 = 0x34,              // 4 key
        KEY_5 = 0x35,              // 5 key
        KEY_6 = 0x36,              // 6 key
        KEY_7 = 0x37,              // 7 key
        KEY_8 = 0x38,              // 8 key
        KEY_9 = 0x39,              // 9 key

        // Alphanumeric Keys
        KEY_A = 0x41,              // A key
        KEY_B = 0x42,              // B key
        KEY_C = 0x43,              // C key
        KEY_D = 0x44,              // D key
        KEY_E = 0x45,              // E key
        KEY_F = 0x46,              // F key
        KEY_G = 0x47,              // G key
        KEY_H = 0x48,              // H key
        KEY_I = 0x49,              // I key
        KEY_J = 0x4A,              // J key
        KEY_K = 0x4B,              // K key
        KEY_L = 0x4C,              // L key
        KEY_M = 0x4D,              // M key
        KEY_N = 0x4E,              // N key
        KEY_O = 0x4F,              // O key
        KEY_P = 0x50,              // P key
        KEY_Q = 0x51,              // Q key
        KEY_R = 0x52,              // R key
        KEY_S = 0x53,              // S key
        KEY_T = 0x54,              // T key
        KEY_U = 0x55,              // U key
        KEY_V = 0x56,              // V key
        KEY_W = 0x57,              // W key
        KEY_X = 0x58,              // X key
        KEY_Y = 0x59,              // Y key
        KEY_Z = 0x5A,              // Z key

        // Other Keys
        KEY_ENTER = 0x0D,           // Enter key
        KEY_SPACE = 0x20,           // Space key
        KEY_SEMICOLON = 0xBA,       // ;
        KEY_EQUAL = 0xBB,           // =, +?
        KEY_COMMA = 0xBC,           // ,
        KEY_MINUS = 0xBD,           // -
        KEY_PERIOD = 0xBE,          // .
        KEY_SLASH = 0xBF,           // /
        KEY_BACKSLASH = 0xDC,       // '\'
        KEY_LEFTBRACKET = 0xDB,     // [
        KEY_RIGHTBRACKET = 0xDD,    // ]
        KEY_APOSTROPHE = 0xDE,      // '

        KEY_UNKNOWN = 0xFF
    };

    enum class KEY_STATE
    {
        NONE,
        PRESS,
        MOVE,
        DOWN,
        UP,
        END
    };

    enum
    {
        KEY_TYPE_COUNT = static_cast<int32>(UINT8_MAX),
        KEY_STATE_COUNT = static_cast<int32>(KEY_STATE::END),
    };

    struct MouseEvent
    {
        MOUSE_TYPE type;
        MOUSE_STATE state;
        int x;
        int y;
    };

    struct KeyboardEvent
    {
        KEY_TYPE type;
        KEY_STATE state;
    };
};
