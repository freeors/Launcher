/*
 * Copyright 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// copy from <aosp>/frameworks/base/services/core/jni/com_android_server_tv_TvUinputBridge.cpp
//

#define LOG_TAG "libkosapi-sendinput"

#include "sendinput.h"

#include <android/keycodes.h>

#include <utils/BitSet.h>
#include <utils/Errors.h>
#include <utils/misc.h>
#include <utils/Log.h>
#include <utils/String8.h>

#include <ctype.h>
#include <linux/input.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <map>
#include <fcntl.h>
#include <linux/uinput.h>
#include <signal.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>

// Refer to EventHub.h
#define MSC_ANDROID_TIME_SEC 0x6
#define MSC_ANDROID_TIME_USEC 0x7

#define SLOT_UNKNOWN -1

/**
 *  \brief The SDL keyboard scancode representation.
 *
 *  Values of this type are used to represent keyboard keys, among other places
 *  in the \link SDL_Keysym::scancode key.keysym.scancode \endlink field of the
 *  SDL_Event structure.
 *
 *  The values in this enumeration are based on the USB usage page standard:
 *  http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
 */
typedef enum
{
    SDL_SCANCODE_UNKNOWN = 0,

    /**
     *  \name Usage page 0x07
     *
     *  These values are from usage page 0x07 (USB keyboard page).
     */
    /* @{ */

    SDL_SCANCODE_A = 4,
    SDL_SCANCODE_B = 5,
    SDL_SCANCODE_C = 6,
    SDL_SCANCODE_D = 7,
    SDL_SCANCODE_E = 8,
    SDL_SCANCODE_F = 9,
    SDL_SCANCODE_G = 10,
    SDL_SCANCODE_H = 11,
    SDL_SCANCODE_I = 12,
    SDL_SCANCODE_J = 13,
    SDL_SCANCODE_K = 14,
    SDL_SCANCODE_L = 15,
    SDL_SCANCODE_M = 16,
    SDL_SCANCODE_N = 17,
    SDL_SCANCODE_O = 18,
    SDL_SCANCODE_P = 19,
    SDL_SCANCODE_Q = 20,
    SDL_SCANCODE_R = 21,
    SDL_SCANCODE_S = 22,
    SDL_SCANCODE_T = 23,
    SDL_SCANCODE_U = 24,
    SDL_SCANCODE_V = 25,
    SDL_SCANCODE_W = 26,
    SDL_SCANCODE_X = 27,
    SDL_SCANCODE_Y = 28,
    SDL_SCANCODE_Z = 29,

    SDL_SCANCODE_1 = 30,
    SDL_SCANCODE_2 = 31,
    SDL_SCANCODE_3 = 32,
    SDL_SCANCODE_4 = 33,
    SDL_SCANCODE_5 = 34,
    SDL_SCANCODE_6 = 35,
    SDL_SCANCODE_7 = 36,
    SDL_SCANCODE_8 = 37,
    SDL_SCANCODE_9 = 38,
    SDL_SCANCODE_0 = 39,

    SDL_SCANCODE_RETURN = 40,
    SDL_SCANCODE_ESCAPE = 41,
    SDL_SCANCODE_BACKSPACE = 42,
    SDL_SCANCODE_TAB = 43,
    SDL_SCANCODE_SPACE = 44,

    SDL_SCANCODE_MINUS = 45,
    SDL_SCANCODE_EQUALS = 46,
    SDL_SCANCODE_LEFTBRACKET = 47,
    SDL_SCANCODE_RIGHTBRACKET = 48,
    SDL_SCANCODE_BACKSLASH = 49, /**< Located at the lower left of the return
                                  *   key on ISO keyboards and at the right end
                                  *   of the QWERTY row on ANSI keyboards.
                                  *   Produces REVERSE SOLIDUS (backslash) and
                                  *   VERTICAL LINE in a US layout, REVERSE
                                  *   SOLIDUS and VERTICAL LINE in a UK Mac
                                  *   layout, NUMBER SIGN and TILDE in a UK
                                  *   Windows layout, DOLLAR SIGN and POUND SIGN
                                  *   in a Swiss German layout, NUMBER SIGN and
                                  *   APOSTROPHE in a German layout, GRAVE
                                  *   ACCENT and POUND SIGN in a French Mac
                                  *   layout, and ASTERISK and MICRO SIGN in a
                                  *   French Windows layout.
                                  */
    SDL_SCANCODE_NONUSHASH = 50, /**< ISO USB keyboards actually use this code
                                  *   instead of 49 for the same key, but all
                                  *   OSes I've seen treat the two codes
                                  *   identically. So, as an implementor, unless
                                  *   your keyboard generates both of those
                                  *   codes and your OS treats them differently,
                                  *   you should generate SDL_SCANCODE_BACKSLASH
                                  *   instead of this code. As a user, you
                                  *   should not rely on this code because SDL
                                  *   will never generate it with most (all?)
                                  *   keyboards.
                                  */
    SDL_SCANCODE_SEMICOLON = 51,
    SDL_SCANCODE_APOSTROPHE = 52,
    SDL_SCANCODE_GRAVE = 53, /**< Located in the top left corner (on both ANSI
                              *   and ISO keyboards). Produces GRAVE ACCENT and
                              *   TILDE in a US Windows layout and in US and UK
                              *   Mac layouts on ANSI keyboards, GRAVE ACCENT
                              *   and NOT SIGN in a UK Windows layout, SECTION
                              *   SIGN and PLUS-MINUS SIGN in US and UK Mac
                              *   layouts on ISO keyboards, SECTION SIGN and
                              *   DEGREE SIGN in a Swiss German layout (Mac:
                              *   only on ISO keyboards), CIRCUMFLEX ACCENT and
                              *   DEGREE SIGN in a German layout (Mac: only on
                              *   ISO keyboards), SUPERSCRIPT TWO and TILDE in a
                              *   French Windows layout, COMMERCIAL AT and
                              *   NUMBER SIGN in a French Mac layout on ISO
                              *   keyboards, and LESS-THAN SIGN and GREATER-THAN
                              *   SIGN in a Swiss German, German, or French Mac
                              *   layout on ANSI keyboards.
                              */
    SDL_SCANCODE_COMMA = 54,
    SDL_SCANCODE_PERIOD = 55,
    SDL_SCANCODE_SLASH = 56,

    SDL_SCANCODE_CAPSLOCK = 57,

    SDL_SCANCODE_F1 = 58,
    SDL_SCANCODE_F2 = 59,
    SDL_SCANCODE_F3 = 60,
    SDL_SCANCODE_F4 = 61,
    SDL_SCANCODE_F5 = 62,
    SDL_SCANCODE_F6 = 63,
    SDL_SCANCODE_F7 = 64,
    SDL_SCANCODE_F8 = 65,
    SDL_SCANCODE_F9 = 66,
    SDL_SCANCODE_F10 = 67,
    SDL_SCANCODE_F11 = 68,
    SDL_SCANCODE_F12 = 69,

    SDL_SCANCODE_PRINTSCREEN = 70,
    SDL_SCANCODE_SCROLLLOCK = 71,
    SDL_SCANCODE_PAUSE = 72,
    SDL_SCANCODE_INSERT = 73, /**< insert on PC, help on some Mac keyboards (but
                                   does send code 73, not 117) */
    SDL_SCANCODE_HOME = 74,
    SDL_SCANCODE_PAGEUP = 75,
    SDL_SCANCODE_DELETE = 76,
    SDL_SCANCODE_END = 77,
    SDL_SCANCODE_PAGEDOWN = 78,
    SDL_SCANCODE_RIGHT = 79,
    SDL_SCANCODE_LEFT = 80,
    SDL_SCANCODE_DOWN = 81,
    SDL_SCANCODE_UP = 82,

    SDL_SCANCODE_NUMLOCKCLEAR = 83, /**< num lock on PC, clear on Mac keyboards
                                     */
    SDL_SCANCODE_KP_DIVIDE = 84,
    SDL_SCANCODE_KP_MULTIPLY = 85,
    SDL_SCANCODE_KP_MINUS = 86,
    SDL_SCANCODE_KP_PLUS = 87,
    SDL_SCANCODE_KP_ENTER = 88,
    SDL_SCANCODE_KP_1 = 89,
    SDL_SCANCODE_KP_2 = 90,
    SDL_SCANCODE_KP_3 = 91,
    SDL_SCANCODE_KP_4 = 92,
    SDL_SCANCODE_KP_5 = 93,
    SDL_SCANCODE_KP_6 = 94,
    SDL_SCANCODE_KP_7 = 95,
    SDL_SCANCODE_KP_8 = 96,
    SDL_SCANCODE_KP_9 = 97,
    SDL_SCANCODE_KP_0 = 98,
    SDL_SCANCODE_KP_PERIOD = 99,

    SDL_SCANCODE_NONUSBACKSLASH = 100, /**< This is the additional key that ISO
                                        *   keyboards have over ANSI ones,
                                        *   located between left shift and Y.
                                        *   Produces GRAVE ACCENT and TILDE in a
                                        *   US or UK Mac layout, REVERSE SOLIDUS
                                        *   (backslash) and VERTICAL LINE in a
                                        *   US or UK Windows layout, and
                                        *   LESS-THAN SIGN and GREATER-THAN SIGN
                                        *   in a Swiss German, German, or French
                                        *   layout. */
    SDL_SCANCODE_APPLICATION = 101, /**< windows contextual menu, compose */
    SDL_SCANCODE_POWER = 102, /**< The USB document says this is a status flag,
                               *   not a physical key - but some Mac keyboards
                               *   do have a power key. */
    SDL_SCANCODE_KP_EQUALS = 103,
    SDL_SCANCODE_F13 = 104,
    SDL_SCANCODE_F14 = 105,
    SDL_SCANCODE_F15 = 106,
    SDL_SCANCODE_F16 = 107,
    SDL_SCANCODE_F17 = 108,
    SDL_SCANCODE_F18 = 109,
    SDL_SCANCODE_F19 = 110,
    SDL_SCANCODE_F20 = 111,
    SDL_SCANCODE_F21 = 112,
    SDL_SCANCODE_F22 = 113,
    SDL_SCANCODE_F23 = 114,
    SDL_SCANCODE_F24 = 115,
    SDL_SCANCODE_EXECUTE = 116,
    SDL_SCANCODE_HELP = 117,
    SDL_SCANCODE_MENU = 118,
    SDL_SCANCODE_SELECT = 119,
    SDL_SCANCODE_STOP = 120,
    SDL_SCANCODE_AGAIN = 121,   /**< redo */
    SDL_SCANCODE_UNDO = 122,
    SDL_SCANCODE_CUT = 123,
    SDL_SCANCODE_COPY = 124,
    SDL_SCANCODE_PASTE = 125,
    SDL_SCANCODE_FIND = 126,
    SDL_SCANCODE_MUTE = 127,
    SDL_SCANCODE_VOLUMEUP = 128,
    SDL_SCANCODE_VOLUMEDOWN = 129,
/* not sure whether there's a reason to enable these */
/*     SDL_SCANCODE_LOCKINGCAPSLOCK = 130,  */
/*     SDL_SCANCODE_LOCKINGNUMLOCK = 131, */
/*     SDL_SCANCODE_LOCKINGSCROLLLOCK = 132, */
    SDL_SCANCODE_KP_COMMA = 133,
    SDL_SCANCODE_KP_EQUALSAS400 = 134,

    SDL_SCANCODE_INTERNATIONAL1 = 135, /**< used on Asian keyboards, see
                                            footnotes in USB doc */
    SDL_SCANCODE_INTERNATIONAL2 = 136,
    SDL_SCANCODE_INTERNATIONAL3 = 137, /**< Yen */
    SDL_SCANCODE_INTERNATIONAL4 = 138,
    SDL_SCANCODE_INTERNATIONAL5 = 139,
    SDL_SCANCODE_INTERNATIONAL6 = 140,
    SDL_SCANCODE_INTERNATIONAL7 = 141,
    SDL_SCANCODE_INTERNATIONAL8 = 142,
    SDL_SCANCODE_INTERNATIONAL9 = 143,
    SDL_SCANCODE_LANG1 = 144, /**< Hangul/English toggle */
    SDL_SCANCODE_LANG2 = 145, /**< Hanja conversion */
    SDL_SCANCODE_LANG3 = 146, /**< Katakana */
    SDL_SCANCODE_LANG4 = 147, /**< Hiragana */
    SDL_SCANCODE_LANG5 = 148, /**< Zenkaku/Hankaku */
    SDL_SCANCODE_LANG6 = 149, /**< reserved */
    SDL_SCANCODE_LANG7 = 150, /**< reserved */
    SDL_SCANCODE_LANG8 = 151, /**< reserved */
    SDL_SCANCODE_LANG9 = 152, /**< reserved */

    SDL_SCANCODE_ALTERASE = 153, /**< Erase-Eaze */
    SDL_SCANCODE_SYSREQ = 154,
    SDL_SCANCODE_CANCEL = 155,
    SDL_SCANCODE_CLEAR = 156,
    SDL_SCANCODE_PRIOR = 157,
    SDL_SCANCODE_RETURN2 = 158,
    SDL_SCANCODE_SEPARATOR = 159,
    SDL_SCANCODE_OUT = 160,
    SDL_SCANCODE_OPER = 161,
    SDL_SCANCODE_CLEARAGAIN = 162,
    SDL_SCANCODE_CRSEL = 163,
    SDL_SCANCODE_EXSEL = 164,

    SDL_SCANCODE_KP_00 = 176,
    SDL_SCANCODE_KP_000 = 177,
    SDL_SCANCODE_THOUSANDSSEPARATOR = 178,
    SDL_SCANCODE_DECIMALSEPARATOR = 179,
    SDL_SCANCODE_CURRENCYUNIT = 180,
    SDL_SCANCODE_CURRENCYSUBUNIT = 181,
    SDL_SCANCODE_KP_LEFTPAREN = 182,
    SDL_SCANCODE_KP_RIGHTPAREN = 183,
    SDL_SCANCODE_KP_LEFTBRACE = 184,
    SDL_SCANCODE_KP_RIGHTBRACE = 185,
    SDL_SCANCODE_KP_TAB = 186,
    SDL_SCANCODE_KP_BACKSPACE = 187,
    SDL_SCANCODE_KP_A = 188,
    SDL_SCANCODE_KP_B = 189,
    SDL_SCANCODE_KP_C = 190,
    SDL_SCANCODE_KP_D = 191,
    SDL_SCANCODE_KP_E = 192,
    SDL_SCANCODE_KP_F = 193,
    SDL_SCANCODE_KP_XOR = 194,
    SDL_SCANCODE_KP_POWER = 195,
    SDL_SCANCODE_KP_PERCENT = 196,
    SDL_SCANCODE_KP_LESS = 197,
    SDL_SCANCODE_KP_GREATER = 198,
    SDL_SCANCODE_KP_AMPERSAND = 199,
    SDL_SCANCODE_KP_DBLAMPERSAND = 200,
    SDL_SCANCODE_KP_VERTICALBAR = 201,
    SDL_SCANCODE_KP_DBLVERTICALBAR = 202,
    SDL_SCANCODE_KP_COLON = 203,
    SDL_SCANCODE_KP_HASH = 204,
    SDL_SCANCODE_KP_SPACE = 205,
    SDL_SCANCODE_KP_AT = 206,
    SDL_SCANCODE_KP_EXCLAM = 207,
    SDL_SCANCODE_KP_MEMSTORE = 208,
    SDL_SCANCODE_KP_MEMRECALL = 209,
    SDL_SCANCODE_KP_MEMCLEAR = 210,
    SDL_SCANCODE_KP_MEMADD = 211,
    SDL_SCANCODE_KP_MEMSUBTRACT = 212,
    SDL_SCANCODE_KP_MEMMULTIPLY = 213,
    SDL_SCANCODE_KP_MEMDIVIDE = 214,
    SDL_SCANCODE_KP_PLUSMINUS = 215,
    SDL_SCANCODE_KP_CLEAR = 216,
    SDL_SCANCODE_KP_CLEARENTRY = 217,
    SDL_SCANCODE_KP_BINARY = 218,
    SDL_SCANCODE_KP_OCTAL = 219,
    SDL_SCANCODE_KP_DECIMAL = 220,
    SDL_SCANCODE_KP_HEXADECIMAL = 221,

    SDL_SCANCODE_LCTRL = 224,
    SDL_SCANCODE_LSHIFT = 225,
    SDL_SCANCODE_LALT = 226, /**< alt, option */
    SDL_SCANCODE_LGUI = 227, /**< windows, command (apple), meta */
    SDL_SCANCODE_RCTRL = 228,
    SDL_SCANCODE_RSHIFT = 229,
    SDL_SCANCODE_RALT = 230, /**< alt gr, option */
    SDL_SCANCODE_RGUI = 231, /**< windows, command (apple), meta */

    SDL_SCANCODE_MODE = 257,    /**< I'm not sure if this is really not covered
                                 *   by any of the above, but since there's a
                                 *   special KMOD_MODE for it I'm adding it here
                                 */

    /* @} *//* Usage page 0x07 */

    /**
     *  \name Usage page 0x0C
     *
     *  These values are mapped from usage page 0x0C (USB consumer page).
     */
    /* @{ */

    SDL_SCANCODE_AUDIONEXT = 258,
    SDL_SCANCODE_AUDIOPREV = 259,
    SDL_SCANCODE_AUDIOSTOP = 260,
    SDL_SCANCODE_AUDIOPLAY = 261,
    SDL_SCANCODE_AUDIOMUTE = 262,
    SDL_SCANCODE_MEDIASELECT = 263,
    SDL_SCANCODE_WWW = 264,
    SDL_SCANCODE_MAIL = 265,
    SDL_SCANCODE_CALCULATOR = 266,
    SDL_SCANCODE_COMPUTER = 267,
    SDL_SCANCODE_AC_SEARCH = 268,
    SDL_SCANCODE_AC_HOME = 269,
    SDL_SCANCODE_AC_BACK = 270,
    SDL_SCANCODE_AC_FORWARD = 271,
    SDL_SCANCODE_AC_STOP = 272,
    SDL_SCANCODE_AC_REFRESH = 273,
    SDL_SCANCODE_AC_BOOKMARKS = 274,

    /* @} *//* Usage page 0x0C */

    /**
     *  \name Walther keys
     *
     *  These are values that Christian Walther added (for mac keyboard?).
     */
    /* @{ */

    SDL_SCANCODE_BRIGHTNESSDOWN = 275,
    SDL_SCANCODE_BRIGHTNESSUP = 276,
    SDL_SCANCODE_DISPLAYSWITCH = 277, /**< display mirroring/dual display
                                           switch, video mode switch */
    SDL_SCANCODE_KBDILLUMTOGGLE = 278,
    SDL_SCANCODE_KBDILLUMDOWN = 279,
    SDL_SCANCODE_KBDILLUMUP = 280,
    SDL_SCANCODE_EJECT = 281,
    SDL_SCANCODE_SLEEP = 282,

    SDL_SCANCODE_APP1 = 283,
    SDL_SCANCODE_APP2 = 284,

    /* @} *//* Walther keys */

    /**
     *  \name Usage page 0x0C (additional media keys)
     *
     *  These values are mapped from usage page 0x0C (USB consumer page).
     */
    /* @{ */

    SDL_SCANCODE_AUDIOREWIND = 285,
    SDL_SCANCODE_AUDIOFASTFORWARD = 286,

    /* @} *//* Usage page 0x0C (additional media keys) */

    /* Add any other keys here. */

    SDL_NUM_SCANCODES = 512 /**< not a key, just marks the number of scancodes
                                 for array bounds */
} SDL_Scancode;

static const SDL_Scancode windows_scancode_table[] =
{
	/*	0						1							2							3							4						5							6							7 */
	/*	8						9							A							B							C						D							E							F */
	SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_ESCAPE,		SDL_SCANCODE_1,				SDL_SCANCODE_2,				SDL_SCANCODE_3,			SDL_SCANCODE_4,				SDL_SCANCODE_5,				SDL_SCANCODE_6,			/* 0 */
	SDL_SCANCODE_7,				SDL_SCANCODE_8,				SDL_SCANCODE_9,				SDL_SCANCODE_0,				SDL_SCANCODE_MINUS,		SDL_SCANCODE_EQUALS,		SDL_SCANCODE_BACKSPACE,		SDL_SCANCODE_TAB,		/* 0 */

	SDL_SCANCODE_Q,				SDL_SCANCODE_W,				SDL_SCANCODE_E,				SDL_SCANCODE_R,				SDL_SCANCODE_T,			SDL_SCANCODE_Y,				SDL_SCANCODE_U,				SDL_SCANCODE_I,			/* 1 */
	SDL_SCANCODE_O,				SDL_SCANCODE_P,				SDL_SCANCODE_LEFTBRACKET,	SDL_SCANCODE_RIGHTBRACKET,	SDL_SCANCODE_RETURN,	SDL_SCANCODE_LCTRL,			SDL_SCANCODE_A,				SDL_SCANCODE_S,			/* 1 */

	SDL_SCANCODE_D,				SDL_SCANCODE_F,				SDL_SCANCODE_G,				SDL_SCANCODE_H,				SDL_SCANCODE_J,			SDL_SCANCODE_K,				SDL_SCANCODE_L,				SDL_SCANCODE_SEMICOLON,	/* 2 */
	SDL_SCANCODE_APOSTROPHE,	SDL_SCANCODE_GRAVE,			SDL_SCANCODE_LSHIFT,		SDL_SCANCODE_BACKSLASH,		SDL_SCANCODE_Z,			SDL_SCANCODE_X,				SDL_SCANCODE_C,				SDL_SCANCODE_V,			/* 2 */

	SDL_SCANCODE_B,				SDL_SCANCODE_N,				SDL_SCANCODE_M,				SDL_SCANCODE_COMMA,			SDL_SCANCODE_PERIOD,	SDL_SCANCODE_SLASH,			SDL_SCANCODE_RSHIFT,		SDL_SCANCODE_PRINTSCREEN,/* 3 */
	SDL_SCANCODE_LALT,			SDL_SCANCODE_SPACE,			SDL_SCANCODE_CAPSLOCK,		SDL_SCANCODE_F1,			SDL_SCANCODE_F2,		SDL_SCANCODE_F3,			SDL_SCANCODE_F4,			SDL_SCANCODE_F5,		/* 3 */

	SDL_SCANCODE_F6,			SDL_SCANCODE_F7,			SDL_SCANCODE_F8,			SDL_SCANCODE_F9,			SDL_SCANCODE_F10,		SDL_SCANCODE_NUMLOCKCLEAR,	SDL_SCANCODE_SCROLLLOCK,	SDL_SCANCODE_HOME,		/* 4 */
	SDL_SCANCODE_UP,			SDL_SCANCODE_PAGEUP,		SDL_SCANCODE_KP_MINUS,		SDL_SCANCODE_LEFT,			SDL_SCANCODE_KP_5,		SDL_SCANCODE_RIGHT,			SDL_SCANCODE_KP_PLUS,		SDL_SCANCODE_END,		/* 4 */

	SDL_SCANCODE_DOWN,			SDL_SCANCODE_PAGEDOWN,		SDL_SCANCODE_INSERT,		SDL_SCANCODE_DELETE,		SDL_SCANCODE_UNKNOWN,	SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_NONUSBACKSLASH,SDL_SCANCODE_F11,		/* 5 */
	SDL_SCANCODE_F12,			SDL_SCANCODE_PAUSE,			SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_LGUI,			SDL_SCANCODE_RGUI,		SDL_SCANCODE_APPLICATION,	SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,	/* 5 */

	SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_F13,		SDL_SCANCODE_F14,			SDL_SCANCODE_F15,			SDL_SCANCODE_F16,		/* 6 */
	SDL_SCANCODE_F17,			SDL_SCANCODE_F18,			SDL_SCANCODE_F19,			SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,	SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,	/* 6 */

	SDL_SCANCODE_INTERNATIONAL2,		SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_INTERNATIONAL1,		SDL_SCANCODE_UNKNOWN,	SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN,	/* 7 */
	SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_INTERNATIONAL4,		SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_INTERNATIONAL5,		SDL_SCANCODE_UNKNOWN,	SDL_SCANCODE_INTERNATIONAL3,		SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN	/* 7 */
};

static SDL_Scancode Android_Keycodes[] = {
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_UNKNOWN */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_SOFT_LEFT */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_SOFT_RIGHT */
    SDL_SCANCODE_AC_HOME, /* AKEYCODE_HOME */
    SDL_SCANCODE_AC_BACK, /* AKEYCODE_BACK */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_CALL */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_ENDCALL */
    SDL_SCANCODE_0, /* AKEYCODE_0 */
    SDL_SCANCODE_1, /* AKEYCODE_1 */
    SDL_SCANCODE_2, /* AKEYCODE_2 */
    SDL_SCANCODE_3, /* AKEYCODE_3 */
    SDL_SCANCODE_4, /* AKEYCODE_4 */
    SDL_SCANCODE_5, /* AKEYCODE_5 */
    SDL_SCANCODE_6, /* AKEYCODE_6 */
    SDL_SCANCODE_7, /* AKEYCODE_7 */
    SDL_SCANCODE_8, /* AKEYCODE_8 */
    SDL_SCANCODE_9, /* AKEYCODE_9 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_STAR */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_POUND */
    SDL_SCANCODE_UP, /* AKEYCODE_DPAD_UP */
    SDL_SCANCODE_DOWN, /* AKEYCODE_DPAD_DOWN */
    SDL_SCANCODE_LEFT, /* AKEYCODE_DPAD_LEFT */
    SDL_SCANCODE_RIGHT, /* AKEYCODE_DPAD_RIGHT */
    SDL_SCANCODE_SELECT, /* AKEYCODE_DPAD_CENTER */
    SDL_SCANCODE_VOLUMEUP, /* AKEYCODE_VOLUME_UP */
    SDL_SCANCODE_VOLUMEDOWN, /* AKEYCODE_VOLUME_DOWN */
    SDL_SCANCODE_POWER, /* AKEYCODE_POWER */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_CAMERA */
    SDL_SCANCODE_CLEAR, /* AKEYCODE_CLEAR */
    SDL_SCANCODE_A, /* AKEYCODE_A */
    SDL_SCANCODE_B, /* AKEYCODE_B */
    SDL_SCANCODE_C, /* AKEYCODE_C */
    SDL_SCANCODE_D, /* AKEYCODE_D */
    SDL_SCANCODE_E, /* AKEYCODE_E */
    SDL_SCANCODE_F, /* AKEYCODE_F */
    SDL_SCANCODE_G, /* AKEYCODE_G */
    SDL_SCANCODE_H, /* AKEYCODE_H */
    SDL_SCANCODE_I, /* AKEYCODE_I */
    SDL_SCANCODE_J, /* AKEYCODE_J */
    SDL_SCANCODE_K, /* AKEYCODE_K */
    SDL_SCANCODE_L, /* AKEYCODE_L */
    SDL_SCANCODE_M, /* AKEYCODE_M */
    SDL_SCANCODE_N, /* AKEYCODE_N */
    SDL_SCANCODE_O, /* AKEYCODE_O */
    SDL_SCANCODE_P, /* AKEYCODE_P */
    SDL_SCANCODE_Q, /* AKEYCODE_Q */
    SDL_SCANCODE_R, /* AKEYCODE_R */
    SDL_SCANCODE_S, /* AKEYCODE_S */
    SDL_SCANCODE_T, /* AKEYCODE_T */
    SDL_SCANCODE_U, /* AKEYCODE_U */
    SDL_SCANCODE_V, /* AKEYCODE_V */
    SDL_SCANCODE_W, /* AKEYCODE_W */
    SDL_SCANCODE_X, /* AKEYCODE_X */
    SDL_SCANCODE_Y, /* AKEYCODE_Y */
    SDL_SCANCODE_Z, /* AKEYCODE_Z */
    SDL_SCANCODE_COMMA, /* AKEYCODE_COMMA */
    SDL_SCANCODE_PERIOD, /* AKEYCODE_PERIOD */
    SDL_SCANCODE_LALT, /* AKEYCODE_ALT_LEFT */
    SDL_SCANCODE_RALT, /* AKEYCODE_ALT_RIGHT */
    SDL_SCANCODE_LSHIFT, /* AKEYCODE_SHIFT_LEFT */
    SDL_SCANCODE_RSHIFT, /* AKEYCODE_SHIFT_RIGHT */
    SDL_SCANCODE_TAB, /* AKEYCODE_TAB */
    SDL_SCANCODE_SPACE, /* AKEYCODE_SPACE */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_SYM */
    SDL_SCANCODE_WWW, /* AKEYCODE_EXPLORER */
    SDL_SCANCODE_MAIL, /* AKEYCODE_ENVELOPE */
    SDL_SCANCODE_RETURN, /* AKEYCODE_ENTER */
    SDL_SCANCODE_BACKSPACE, /* AKEYCODE_DEL */
    SDL_SCANCODE_GRAVE, /* AKEYCODE_GRAVE */
    SDL_SCANCODE_MINUS, /* AKEYCODE_MINUS */
    SDL_SCANCODE_EQUALS, /* AKEYCODE_EQUALS */
    SDL_SCANCODE_LEFTBRACKET, /* AKEYCODE_LEFT_BRACKET */
    SDL_SCANCODE_RIGHTBRACKET, /* AKEYCODE_RIGHT_BRACKET */
    SDL_SCANCODE_BACKSLASH, /* AKEYCODE_BACKSLASH */
    SDL_SCANCODE_SEMICOLON, /* AKEYCODE_SEMICOLON */
    SDL_SCANCODE_APOSTROPHE, /* AKEYCODE_APOSTROPHE */
    SDL_SCANCODE_SLASH, /* AKEYCODE_SLASH */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_AT */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_NUM */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_HEADSETHOOK */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_FOCUS */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_PLUS */
    SDL_SCANCODE_MENU, /* AKEYCODE_MENU */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_NOTIFICATION */
    SDL_SCANCODE_AC_SEARCH, /* AKEYCODE_SEARCH */
    SDL_SCANCODE_AUDIOPLAY, /* AKEYCODE_MEDIA_PLAY_PAUSE */
    SDL_SCANCODE_AUDIOSTOP, /* AKEYCODE_MEDIA_STOP */
    SDL_SCANCODE_AUDIONEXT, /* AKEYCODE_MEDIA_NEXT */
    SDL_SCANCODE_AUDIOPREV, /* AKEYCODE_MEDIA_PREVIOUS */
    SDL_SCANCODE_AUDIOREWIND, /* AKEYCODE_MEDIA_REWIND */
    SDL_SCANCODE_AUDIOFASTFORWARD, /* AKEYCODE_MEDIA_FAST_FORWARD */
    SDL_SCANCODE_MUTE, /* AKEYCODE_MUTE */
    SDL_SCANCODE_PAGEUP, /* AKEYCODE_PAGE_UP */
    SDL_SCANCODE_PAGEDOWN, /* AKEYCODE_PAGE_DOWN */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_PICTSYMBOLS */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_SWITCH_CHARSET */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_A */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_B */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_C */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_X */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_Y */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_Z */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_L1 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_R1 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_L2 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_R2 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_THUMBL */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_THUMBR */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_START */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_SELECT */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_MODE */
    SDL_SCANCODE_ESCAPE, /* AKEYCODE_ESCAPE */
    SDL_SCANCODE_DELETE, /* AKEYCODE_FORWARD_DEL */
    SDL_SCANCODE_LCTRL, /* AKEYCODE_CTRL_LEFT */
    SDL_SCANCODE_RCTRL, /* AKEYCODE_CTRL_RIGHT */
    SDL_SCANCODE_CAPSLOCK, /* AKEYCODE_CAPS_LOCK */
    SDL_SCANCODE_SCROLLLOCK, /* AKEYCODE_SCROLL_LOCK */
    SDL_SCANCODE_LGUI, /* AKEYCODE_META_LEFT */
    SDL_SCANCODE_RGUI, /* AKEYCODE_META_RIGHT */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_FUNCTION */
    SDL_SCANCODE_PRINTSCREEN, /* AKEYCODE_SYSRQ */
    SDL_SCANCODE_PAUSE, /* AKEYCODE_BREAK */
    SDL_SCANCODE_HOME, /* AKEYCODE_MOVE_HOME */
    SDL_SCANCODE_END, /* AKEYCODE_MOVE_END */
    SDL_SCANCODE_INSERT, /* AKEYCODE_INSERT */
    SDL_SCANCODE_AC_FORWARD, /* AKEYCODE_FORWARD */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_MEDIA_PLAY */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_MEDIA_PAUSE */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_MEDIA_CLOSE */
    SDL_SCANCODE_EJECT, /* AKEYCODE_MEDIA_EJECT */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_MEDIA_RECORD */
    SDL_SCANCODE_F1, /* AKEYCODE_F1 */
    SDL_SCANCODE_F2, /* AKEYCODE_F2 */
    SDL_SCANCODE_F3, /* AKEYCODE_F3 */
    SDL_SCANCODE_F4, /* AKEYCODE_F4 */
    SDL_SCANCODE_F5, /* AKEYCODE_F5 */
    SDL_SCANCODE_F6, /* AKEYCODE_F6 */
    SDL_SCANCODE_F7, /* AKEYCODE_F7 */
    SDL_SCANCODE_F8, /* AKEYCODE_F8 */
    SDL_SCANCODE_F9, /* AKEYCODE_F9 */
    SDL_SCANCODE_F10, /* AKEYCODE_F10 */
    SDL_SCANCODE_F11, /* AKEYCODE_F11 */
    SDL_SCANCODE_F12, /* AKEYCODE_F12 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_NUM_LOCK */
    SDL_SCANCODE_KP_0, /* AKEYCODE_NUMPAD_0 */
    SDL_SCANCODE_KP_1, /* AKEYCODE_NUMPAD_1 */
    SDL_SCANCODE_KP_2, /* AKEYCODE_NUMPAD_2 */
    SDL_SCANCODE_KP_3, /* AKEYCODE_NUMPAD_3 */
    SDL_SCANCODE_KP_4, /* AKEYCODE_NUMPAD_4 */
    SDL_SCANCODE_KP_5, /* AKEYCODE_NUMPAD_5 */
    SDL_SCANCODE_KP_6, /* AKEYCODE_NUMPAD_6 */
    SDL_SCANCODE_KP_7, /* AKEYCODE_NUMPAD_7 */
    SDL_SCANCODE_KP_8, /* AKEYCODE_NUMPAD_8 */
    SDL_SCANCODE_KP_9, /* AKEYCODE_NUMPAD_9 */
    SDL_SCANCODE_KP_DIVIDE, /* AKEYCODE_NUMPAD_DIVIDE */
    SDL_SCANCODE_KP_MULTIPLY, /* AKEYCODE_NUMPAD_MULTIPLY */
    SDL_SCANCODE_KP_MINUS, /* AKEYCODE_NUMPAD_SUBTRACT */
    SDL_SCANCODE_KP_PLUS, /* AKEYCODE_NUMPAD_ADD */
    SDL_SCANCODE_KP_PERIOD, /* AKEYCODE_NUMPAD_DOT */
    SDL_SCANCODE_KP_COMMA, /* AKEYCODE_NUMPAD_COMMA */
    SDL_SCANCODE_KP_ENTER, /* AKEYCODE_NUMPAD_ENTER */
    SDL_SCANCODE_KP_EQUALS, /* AKEYCODE_NUMPAD_EQUALS */
    SDL_SCANCODE_KP_LEFTPAREN, /* AKEYCODE_NUMPAD_LEFT_PAREN */
    SDL_SCANCODE_KP_RIGHTPAREN, /* AKEYCODE_NUMPAD_RIGHT_PAREN */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_VOLUME_MUTE */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_INFO */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_CHANNEL_UP */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_CHANNEL_DOWN */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_ZOOM_IN */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_ZOOM_OUT */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_WINDOW */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_GUIDE */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_DVR */
    SDL_SCANCODE_AC_BOOKMARKS, /* AKEYCODE_BOOKMARK */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_CAPTIONS */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_SETTINGS */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_POWER */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_INPUT */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_STB_POWER */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_STB_INPUT */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_AVR_POWER */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_AVR_INPUT */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_PROG_RED */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_PROG_GREEN */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_PROG_YELLOW */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_PROG_BLUE */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_APP_SWITCH */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_1 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_2 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_3 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_4 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_5 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_6 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_7 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_8 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_9 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_10 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_11 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_12 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_13 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_14 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_15 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_BUTTON_16 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_LANGUAGE_SWITCH */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_MANNER_MODE */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_3D_MODE */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_CONTACTS */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_CALENDAR */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_MUSIC */
    SDL_SCANCODE_CALCULATOR, /* AKEYCODE_CALCULATOR */
    SDL_SCANCODE_LANG5, /* AKEYCODE_ZENKAKU_HANKAKU */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_EISU */
    SDL_SCANCODE_INTERNATIONAL5, /* AKEYCODE_MUHENKAN */
    SDL_SCANCODE_INTERNATIONAL4, /* AKEYCODE_HENKAN */
    SDL_SCANCODE_LANG3, /* AKEYCODE_KATAKANA_HIRAGANA */
    SDL_SCANCODE_INTERNATIONAL3, /* AKEYCODE_YEN */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_RO */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_KANA */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_ASSIST */
    SDL_SCANCODE_BRIGHTNESSDOWN, /* AKEYCODE_BRIGHTNESS_DOWN */
    SDL_SCANCODE_BRIGHTNESSUP, /* AKEYCODE_BRIGHTNESS_UP */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_MEDIA_AUDIO_TRACK */
    SDL_SCANCODE_SLEEP, /* AKEYCODE_SLEEP */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_WAKEUP */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_PAIRING */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_MEDIA_TOP_MENU */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_11 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_12 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_LAST_CHANNEL */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_DATA_SERVICE */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_VOICE_ASSIST */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_RADIO_SERVICE */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_TELETEXT */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_NUMBER_ENTRY */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_TERRESTRIAL_ANALOG */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_TERRESTRIAL_DIGITAL */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_SATELLITE */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_SATELLITE_BS */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_SATELLITE_CS */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_SATELLITE_SERVICE */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_NETWORK */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_ANTENNA_CABLE */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_INPUT_HDMI_1 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_INPUT_HDMI_2 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_INPUT_HDMI_3 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_INPUT_HDMI_4 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_INPUT_COMPOSITE_1 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_INPUT_COMPOSITE_2 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_INPUT_COMPONENT_1 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_INPUT_COMPONENT_2 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_INPUT_VGA_1 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_AUDIO_DESCRIPTION */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_AUDIO_DESCRIPTION_MIX_UP */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_AUDIO_DESCRIPTION_MIX_DOWN */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_ZOOM_MODE */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_CONTENTS_MENU */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_MEDIA_CONTEXT_MENU */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_TV_TIMER_PROGRAMMING */
    SDL_SCANCODE_HELP, /* AKEYCODE_HELP */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_NAVIGATE_PREVIOUS */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_NAVIGATE_NEXT */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_NAVIGATE_IN */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_NAVIGATE_OUT */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_STEM_PRIMARY */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_STEM_1 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_STEM_2 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_STEM_3 */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_DPAD_UP_LEFT */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_DPAD_DOWN_LEFT */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_DPAD_UP_RIGHT */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_DPAD_DOWN_RIGHT */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_MEDIA_SKIP_FORWARD */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_MEDIA_SKIP_BACKWARD */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_MEDIA_STEP_FORWARD */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_MEDIA_STEP_BACKWARD */
    SDL_SCANCODE_UNKNOWN, /* AKEYCODE_SOFT_SLEEP */
    SDL_SCANCODE_CUT, /* AKEYCODE_CUT */
    SDL_SCANCODE_COPY, /* AKEYCODE_COPY */
    SDL_SCANCODE_PASTE, /* AKEYCODE_PASTE */
};

int win_scan_code_2_android_scan_code(int win_code)
{
    // ALOGI("win_scan_code_2_android_scan_code--- win_code: %i", win_code);
    SDL_Scancode usb_code = SDL_SCANCODE_UNKNOWN;
    if (win_code >= 0 || win_code <= 127) {
        usb_code = windows_scancode_table[win_code];
    }
    if (usb_code == SDL_SCANCODE_UNKNOWN) {
        ALOGI("---win_scan_code_2_android_scan_code, usb_code == SDL_SCANCODE_UNKNOWN, return -1");
        return -1;
    }
    int size = sizeof(Android_Keycodes) / sizeof(Android_Keycodes[0]);
    // ALOGI("win_scan_code_2_android_scan_code, size: %i, usb_code: %i", size, (int)usb_code);
    for (int n = 0; n < size; n ++) {
        if (Android_Keycodes[n] == usb_code) {
            // ALOGI("---win_scan_code_2_android_scan_code, %i ==> %i", usb_code, n);
            return n;
        }
    }
    // ALOGI("---win_scan_code_2_android_scan_code, cannnot find, return -1");
    return -1;
}

namespace android {

static std::map<int32_t,int> keysMap;

static void initKeysMap() {
    if (keysMap.empty()) {
        for (size_t i = 0; i < NELEM(KEYS); i++) {
            keysMap[KEYS[i].androidKeyCode] = KEYS[i].linuxKeyCode;
        }
    }
}

static int32_t getLinuxKeyCode(int32_t androidKeyCode) {
    std::map<int,int>::iterator it = keysMap.find(androidKeyCode);
    if (it != keysMap.end()) {
        return it->second;
    }
    return KEY_UNKNOWN;
}

NativeConnection::NativeConnection(int fd, int32_t maxPointers) :
    mFd(fd)
    , mMaxPointers(maxPointers)
    , lastX(0)
    , lastY(0) {
    ALOGI("NativeConnection::NativeConnectione--- fd: %d maxPointers: %i", mFd, maxPointers);
    // nativeSendPointerMove(-1, -1);
    // sendEvent(EV_SYN, SYN_REPORT, 0);
}

NativeConnection::~NativeConnection() {
    ALOGI("Un-Registering uinput device %d.", mFd);
    nativeClear();
    ioctl(mFd, UI_DEV_DESTROY);
    close(mFd);
}

NativeConnection* NativeConnection::open(const char* name, const char* uniqueId,
        bool keyboard, int32_t screenWidth, int32_t screenHeight) {
    ALOGI("Registering uinput device %s: keyboard: %s, touch pad size %dx%d", name, keyboard? "true": "false", screenWidth, screenHeight);
    int32_t maxPointers = 1;

    int fd = ::open("/dev/uinput", O_WRONLY | O_NDELAY);
    if (fd < 0) {
        ALOGE("Cannot open /dev/uinput: %s.", strerror(errno));
        return nullptr;
    }

    struct uinput_user_dev uinp;
    memset(&uinp, 0, sizeof(struct uinput_user_dev));
    strlcpy(uinp.name, name, UINPUT_MAX_NAME_SIZE);
    uinp.id.version = 1;
    uinp.id.bustype = BUS_VIRTUAL;
    uinp.absmin[ABS_X] = 0;
    uinp.absmax[ABS_X] = screenWidth - 1;
    uinp.absmin[ABS_Y] = 0;
    uinp.absmax[ABS_Y] = screenHeight - 1;

    // initialize keymap
    initKeysMap();

    // write device unique id to the phys property
    ioctl(fd, UI_SET_PHYS, uniqueId);

    // set the keys mapped
    ioctl(fd, UI_SET_EVBIT, EV_KEY);

    if (keyboard) {
        // make android has AINPUT_SOURCE_KEYBOARD flag ==> import KeyboardInputMapper
        for (size_t i = 0; i < NELEM(KEYS); i++) {
            ioctl(fd, UI_SET_KEYBIT, KEYS[i].linuxKeyCode);
        }
    }

    ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH);

    // set abs events maps
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    ioctl(fd, UI_SET_ABSBIT, ABS_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_Y);

    // when TouchInputMapper, let support BTN_RIGHT
    ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
/*
    // support CursorInputMapper
    ioctl(fd, UI_SET_KEYBIT, BTN_MOUSE);
    {
        ioctl(fd, UI_SET_EVBIT, EV_REL);
        ioctl(fd, UI_SET_RELBIT, REL_X);
        ioctl(fd, UI_SET_RELBIT, REL_Y);
        ioctl(fd, UI_SET_RELBIT, REL_WHEEL);
        // when CursorInputMapper, require support BTN_RIGHT
        ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
    }
*/
    {
        ioctl(fd, UI_SET_EVBIT, EV_REL);
        ioctl(fd, UI_SET_RELBIT, REL_WHEEL);
        ioctl(fd, UI_SET_RELBIT, REL_HWHEEL);
    }

    ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);

    // set the misc events maps
    // ioctl(fd, UI_SET_EVBIT, EV_ABS);
    // ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
    // ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);


    // register the input device
    if (write(fd, &uinp, sizeof(uinp)) != sizeof(uinp)) {
        ALOGE("Cannot write uinput_user_dev to fd %d: %s.", fd, strerror(errno));
        close(fd);
        return NULL;
    }
    if (ioctl(fd, UI_DEV_CREATE) != 0) {
        ALOGE("Unable to create uinput device: %s.", strerror(errno));
        close(fd);
        return nullptr;
    }

    ALOGD("Created uinput device, fd=%d.", fd);
    return new NativeConnection(fd, maxPointers);
}

void NativeConnection::sendEvent(int32_t type, int32_t code, int32_t value) {
    struct input_event iev;
    memset(&iev, 0, sizeof(iev));
    iev.type = type;
    iev.code = code;
    iev.value = value;
    write(mFd, &iev, sizeof(iev));
}

void NativeConnection::nativeSendTimestamp(long timestamp) {
    sendEvent(EV_MSC, MSC_ANDROID_TIME_SEC, timestamp / 1000L);
    sendEvent(EV_MSC, MSC_ANDROID_TIME_USEC, (timestamp % 1000L) * 1000L);
}

void NativeConnection::nativeSendKey(int keyCode, bool down) {
    int32_t code = getLinuxKeyCode(keyCode);
    if (code != KEY_UNKNOWN) {
        // ALOGI("nativeSendKey(keyCode: %d, down: %s)", keyCode, down? "true": "false");
        sendEvent(EV_KEY, code, down? 1 : 0);

    } else {
        ALOGE("Received an unknown keycode of %d.", keyCode);
    }
}

void NativeConnection::nativeSendPointerDown(int pointerId, int x, int y) {
    // ALOGI("NativeConnection::nativeSendPointerDown--- pointerId: %d x: %i y: %i", pointerId, x, y);

    if (pointerId == 0) {
        sendEvent(EV_KEY, BTN_TOUCH, 1);
    } else if (pointerId == 1) {
        sendEvent(EV_KEY, BTN_RIGHT, 1);
    }
    if (x != -1) {
        sendEvent(EV_ABS, ABS_X, x);
    }
    if (y != -1) {
        sendEvent(EV_ABS, ABS_Y, y);
    }
    // ALOGI("---NativeConnection::nativeSendPointerDown X");
}

void NativeConnection::nativeSendPointerUp(int pointerId, int x, int y) {
    // ALOGI("NativeConnection::nativeSendPointerUp--- pointerId: %d, x: %i, y: %i", pointerId, x, y);

    if (pointerId == 0) {
        sendEvent(EV_KEY, BTN_TOUCH, 0);
    } else if (pointerId == 1) {
        sendEvent(EV_KEY, BTN_RIGHT, 0);
    }
    if (x != -1) {
        sendEvent(EV_ABS, ABS_X, x);
    }
    if (y != -1) {
        sendEvent(EV_ABS, ABS_Y, y);
    }

    // ALOGI("---NativeConnection::nativeSendPointerUp X");
}

bool NativeConnection::nativeSendPointerMove(int x, int y)
{
    sendEvent(EV_ABS, ABS_X, x);
    sendEvent(EV_ABS, ABS_Y, y);
    return true;
}

void NativeConnection::nativeSendWheel(bool vertical, int val)
{
    sendEvent(EV_REL, vertical? REL_WHEEL: REL_HWHEEL, val > 0? 1: -1);
}

void NativeConnection::nativeSendPointerSync() {
    sendEvent(EV_SYN, SYN_REPORT, 0);
}

void NativeConnection::nativeClear() {
    nativeSendPointerUp(1, -1, -1);
    // nativeSendPointerUp(0, -1, -1);

    // Clear keys.
    for (size_t i = 0; i < NELEM(KEYS); i++) {
        sendEvent(EV_KEY, KEYS[i].linuxKeyCode, 0);
    }

    // Sync pointer events
    sendEvent(EV_SYN, SYN_REPORT, 0);
}

uint32_t NativeConnection::send_input(uint32_t input_count, KosInput* inputs)
{
    bool requireSend = false;
    for (int n = 0; n < (int)input_count; n ++) {
        KosInput* src = inputs + n;
        if (src->type == KOS_INPUT_MOUSE) {
            int x = -1;
            int y = -1;
            if (src->u.mi.flags & MOUSEEVENTF_ABSOLUTE) {
                x = src->u.mi.dx;
                y = src->u.mi.dy;
            }
            if (src->u.mi.flags & MOUSEEVENTF_LEFTDOWN) {
                requireSend = true;
                nativeSendPointerDown(0, x, y);

            } else if (src->u.mi.flags & MOUSEEVENTF_LEFTUP) {
                requireSend = true;
                nativeSendPointerUp(0, x, y);

            } else if (src->u.mi.flags & MOUSEEVENTF_RIGHTDOWN) {
                requireSend = true;
                nativeSendPointerDown(1, -1, -1);

            } else if (src->u.mi.flags & MOUSEEVENTF_RIGHTUP) {
                requireSend = true;
                nativeSendPointerUp(1, -1, -1);

            } else if (src->u.mi.flags & MOUSEEVENTF_WHEEL) {
                if (src->u.mi.mouse_data != 0) {
                    requireSend = true;
                    nativeSendWheel(true, (int32_t)src->u.mi.mouse_data);
                }

            } else if (src->u.mi.flags & MOUSEEVENTF_HWHEEL) {
                if (src->u.mi.mouse_data != 0) {
                    requireSend = true;
                    nativeSendWheel(false, (int32_t)src->u.mi.mouse_data);
                }

            } else if (x != -1 && y != -1) {
                requireSend = true;
                nativeSendPointerMove(x, y);
            }
        } else if (src->type == KOS_INPUT_KEYBOARD) {
            int android_scan_code = win_scan_code_2_android_scan_code(src->u.ki.scan_code);
            if (android_scan_code != -1) {
                requireSend = true;
                nativeSendKey(android_scan_code, src->u.ki.flags & KEYEVENTF_KEYUP? false: true);
            }
        }
    }
    if (requireSend) {
        sendEvent(EV_SYN, SYN_REPORT, 0);
    }
    return input_count;
}

} // namespace android
