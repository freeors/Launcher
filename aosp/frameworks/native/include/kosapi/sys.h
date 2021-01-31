#ifndef _LIBKOSAPI_SYS_H
#define _LIBKOSAPI_SYS_H

#ifndef _WIN32
#include <sys/cdefs.h>
#include <stdint.h>
#include <stdbool.h>
#else
#include <SDL_config.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void kosGetVersion(char* ver, int max_bytes);
bool kosRunApp(const char* package);

typedef struct {
	char res_path[64];
	char userdata_path[128];
} KosOsInfo;
void kosGetOsInfo(KosOsInfo* info);

typedef void (*fdid_sys2_touched)(int touch_device_id, int pointer_finter_id, int action, float x, float y, float p);
typedef void (*fdid_sys2_hover_moved)(int state, int action, float x, float y);

void kosSetSysDid(fdid_sys2_touched did_touched, fdid_sys2_hover_moved did_hover_moved);

void kosPumpEvent();

bool kosCreateInput(bool keyboard, int screen_width, int screen_height);
void kosDestroyInput();

#ifndef _WIN32
//
// keyboard section
//
#define KEYEVENTF_EXTENDEDKEY 0x0001
#define KEYEVENTF_KEYUP       0x0002
#define KEYEVENTF_UNICODE     0x0004
#define KEYEVENTF_SCANCODE    0x0008

//
// mouse section
//
#define MOUSEEVENTF_MOVE        0x0001 /* mouse move */
#define MOUSEEVENTF_LEFTDOWN    0x0002 /* left button down */
#define MOUSEEVENTF_LEFTUP      0x0004 /* left button up */
#define MOUSEEVENTF_RIGHTDOWN   0x0008 /* right button down */
#define MOUSEEVENTF_RIGHTUP     0x0010 /* right button up */
#define MOUSEEVENTF_MIDDLEDOWN  0x0020 /* middle button down */
#define MOUSEEVENTF_MIDDLEUP    0x0040 /* middle button up */
#define MOUSEEVENTF_XDOWN       0x0080 /* x button down */
#define MOUSEEVENTF_XUP         0x0100 /* x button down */
#define MOUSEEVENTF_WHEEL                0x0800 /* wheel button rolled */
#define MOUSEEVENTF_HWHEEL              0x01000 /* hwheel button rolled */
#define MOUSEEVENTF_MOVE_NOCOALESCE      0x2000 /* do not coalesce mouse moves */
#define MOUSEEVENTF_VIRTUALDESK          0x4000 /* map to entire virtual desktop */
#define MOUSEEVENTF_ABSOLUTE             0x8000 /* absolute move */

/* XButton values are WORD flags */
#define XBUTTON1      0x0001
#define XBUTTON2      0x0002
#endif

typedef struct {
    int    dx;
    int    dy;
    uint32_t   mouse_data;
    uint32_t   flags;
    uint32_t   time;
    // ULONG_PTR  dwExtraInfo;
} KosMouseInput;

typedef struct {
    uint16_t   virtual_key;
    uint16_t   scan_code;
    uint32_t   flags;
    uint32_t   time;
    // ULONG_PTR dwExtraInfo;
} KosKeybdInput;

#define KOS_INPUT_MOUSE     0
#define KOS_INPUT_KEYBOARD  1
#define KOS_INPUT_HARDWARE  2

typedef struct {
    uint32_t   type;
    union
    {
        KosMouseInput      mi;
        KosKeybdInput      ki;
    } u;
} KosInput;

uint32_t kosSendInput(uint32_t input_count, // number of input in the array
    KosInput* inputs);  // array of inputs

#ifdef __cplusplus
}
#endif

#endif /* _LIBKOSAPI_SYS_H */
