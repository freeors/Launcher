#ifndef LIBKOSAPI_SENDINPUT_H_
#define LIBKOSAPI_SENDINPUT_H_

#include <android/keycodes.h>
#include <linux/input.h>

#include <kosapi/sys.h>

namespace android {

// Map the keys specified in virtual-remote.kl.
// Only specify the keys actually used in the layout here.
struct Key {
    int linuxKeyCode;
    int32_t androidKeyCode;
};

// List of all of the keycodes that the emote is capable of sending.
// sort by linuxKeyCode that from <aosp>/kernel/include/dt-bindings/input/linux-event-codes.h
static Key KEYS[] = {
    { KEY_ESC, AKEYCODE_ESCAPE },

    { KEY_1, AKEYCODE_1 },
    { KEY_2, AKEYCODE_2 },
    { KEY_3, AKEYCODE_3 },
    { KEY_4, AKEYCODE_4 },
    { KEY_5, AKEYCODE_5 },
    { KEY_6, AKEYCODE_6 },
    { KEY_7, AKEYCODE_7 },
    { KEY_8, AKEYCODE_8 },
    { KEY_9, AKEYCODE_9 },
    { KEY_0, AKEYCODE_0 },

    { KEY_MINUS, AKEYCODE_MINUS},
    { KEY_EQUAL, AKEYCODE_EQUALS},
    { KEY_BACKSPACE, AKEYCODE_DEL},
    { KEY_TAB, AKEYCODE_TAB},
    { KEY_Q, AKEYCODE_Q},
    { KEY_W, AKEYCODE_W},
    { KEY_E, AKEYCODE_E},
    { KEY_R, AKEYCODE_R},
    { KEY_T, AKEYCODE_T},
    { KEY_Y, AKEYCODE_Y},
    { KEY_U, AKEYCODE_U},
    { KEY_I, AKEYCODE_I},
    { KEY_O, AKEYCODE_O},
    { KEY_P, AKEYCODE_P},
    { KEY_LEFTBRACE, AKEYCODE_LEFT_BRACKET},
    { KEY_RIGHTBRACE, AKEYCODE_RIGHT_BRACKET},
    { KEY_ENTER, AKEYCODE_ENTER},
    { KEY_LEFTCTRL, AKEYCODE_CTRL_LEFT},
    { KEY_A, AKEYCODE_A},
    { KEY_S, AKEYCODE_S},
    { KEY_D, AKEYCODE_D},
    { KEY_F, AKEYCODE_F},
    { KEY_G, AKEYCODE_G},
    { KEY_H, AKEYCODE_H},
    { KEY_J, AKEYCODE_J},
    { KEY_K, AKEYCODE_K},
    { KEY_L, AKEYCODE_L},
    { KEY_SEMICOLON, AKEYCODE_SEMICOLON},
    { KEY_APOSTROPHE, AKEYCODE_APOSTROPHE},
    { KEY_GRAVE, AKEYCODE_GRAVE},
    { KEY_LEFTSHIFT, AKEYCODE_SHIFT_LEFT},
    { KEY_BACKSLASH, AKEYCODE_BACKSLASH},
    { KEY_Z, AKEYCODE_Z},
    { KEY_X, AKEYCODE_X},
    { KEY_C, AKEYCODE_C},
    { KEY_V, AKEYCODE_V},
    { KEY_B, AKEYCODE_B},
    { KEY_N, AKEYCODE_N},
    { KEY_M, AKEYCODE_M},
    { KEY_COMMA, AKEYCODE_COMMA},
    { KEY_DOT, AKEYCODE_PERIOD},
    { KEY_SLASH, AKEYCODE_SLASH},
    { KEY_RIGHTSHIFT, AKEYCODE_SHIFT_RIGHT},
    { KEY_KPASTERISK, AKEYCODE_STAR},
    { KEY_LEFTALT, AKEYCODE_ALT_LEFT},
    { KEY_SPACE, AKEYCODE_SPACE},
    { KEY_CAPSLOCK, AKEYCODE_CAPS_LOCK},

    { KEY_F1, AKEYCODE_F1},
    { KEY_F2, AKEYCODE_F2},
    { KEY_F3, AKEYCODE_F3},
    { KEY_F4, AKEYCODE_F4},
    { KEY_F5, AKEYCODE_F5},
    { KEY_F6, AKEYCODE_F6},
    { KEY_F7, AKEYCODE_F7},
    { KEY_F8, AKEYCODE_F8},
    { KEY_F9, AKEYCODE_F9},
    { KEY_F10, AKEYCODE_F10},

    { KEY_NUMLOCK, AKEYCODE_NUM_LOCK},
    { KEY_SCROLLLOCK, AKEYCODE_SCROLL_LOCK},
    { KEY_KP7, AKEYCODE_NUMPAD_7},
    { KEY_KP8, AKEYCODE_NUMPAD_8},
    { KEY_KP9, AKEYCODE_NUMPAD_9},
    { KEY_KPMINUS, AKEYCODE_NUMPAD_SUBTRACT},
    { KEY_KP4, AKEYCODE_NUMPAD_4},
    { KEY_KP5, AKEYCODE_NUMPAD_5},
    { KEY_KP6, AKEYCODE_NUMPAD_6},
    { KEY_KPPLUS, AKEYCODE_NUMPAD_ADD},
    { KEY_KP1, AKEYCODE_NUMPAD_1},
    { KEY_KP2, AKEYCODE_NUMPAD_2},
    { KEY_KP3, AKEYCODE_NUMPAD_3},
    { KEY_KP0, AKEYCODE_NUMPAD_0},
    { KEY_KPDOT, AKEYCODE_NUMPAD_DOT},
/*
    { KEY_ZENKAKUHANKAKU	85
    { KEY_102ND		86
*/
    { KEY_F11, AKEYCODE_F11},
    { KEY_F12, AKEYCODE_F12},
/*
    { KEY_RO			89
    { KEY_KATAKANA		90
    { KEY_HIRAGANA		91
    { KEY_HENKAN		92
    { KEY_KATAKANAHIRAGANA	93
    { KEY_MUHENKAN		94
    { KEY_KPJPCOMMA		95
    { KEY_KPENTER		96
*/
    { KEY_RIGHTCTRL, AKEYCODE_CTRL_RIGHT},
/*
    { KEY_KPSLASH		98
    { KEY_SYSRQ		99
*/
    { KEY_RIGHTALT, AKEYCODE_ALT_RIGHT},
/*
    { KEY_LINEFEED		101
    { KEY_HOME		102
*/
    { KEY_UP, AKEYCODE_DPAD_UP},
    { KEY_PAGEUP, AKEYCODE_PAGE_UP},
    { KEY_LEFT, AKEYCODE_DPAD_LEFT},
    { KEY_RIGHT, AKEYCODE_DPAD_RIGHT},
    { KEY_END, AKEYCODE_MOVE_END},
    { KEY_DOWN, AKEYCODE_DPAD_DOWN},
    { KEY_PAGEDOWN, AKEYCODE_PAGE_DOWN},
    { KEY_INSERT, AKEYCODE_INSERT},
    { KEY_DELETE, AKEYCODE_FORWARD_DEL},
};

class NativeConnection {
public:
    ~NativeConnection();

    // @screenWidth, screenHeight: width/height when orientation is DISPLAY_ORIENTATION_0 although current in DISPLAY_ORIENTATION_90/270.
    static NativeConnection* open(const char* name, const char* uniqueId,
            bool keyboard, int32_t screenWidth, int32_t screenHeight);

    void sendEvent(int32_t type, int32_t code, int32_t value);

    int32_t getMaxPointers() const { return mMaxPointers; }

    void nativeSendTimestamp(long timestamp);
    void nativeSendKey(int keyCode, bool down);
    void nativeSendPointerDown(int pointerId, int x, int y);
    void nativeSendPointerUp(int pointerId, int x, int y);
    bool nativeSendPointerMove(int x, int y);
    void nativeSendPointerSync();
    void nativeSendWheel(bool vertical, int val);
    void nativeClear();
    uint32_t send_input(uint32_t input_count, KosInput* inputs);

private:
    NativeConnection(int fd, int32_t maxPointers);

    const int mFd;
    const int32_t mMaxPointers;
    int lastX;
    int lastY;
};

} // namespace android

#endif // LIBKOSAPI_SENDINPUT_H_
