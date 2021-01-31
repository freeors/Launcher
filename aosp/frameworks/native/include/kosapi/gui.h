#ifndef _LIBKOSAPI_GUI_H
#define _LIBKOSAPI_GUI_H

#ifndef _WIN32
#include <sys/cdefs.h>
#include <stdint.h>
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct ANativeWindow;

ANativeWindow* kosGetSurface();

#define KOS_RECORDSCREEN_FLAG_ORIENTATION_MASK	0x7 // not use 0x3, orientation maybe increase in future.
#define KOS_RECORDSCREEN_FLAG_SYNCFRAME 0x8

// Both KOS_DISPLAY_ORIENTATION_X and KosDisplayInfo are derived from <aosp>/frameworks/native/include/ui/DisplayInfo.h
#define KOS_DISPLAY_ORIENTATION_0		0
#define KOS_DISPLAY_ORIENTATION_90		1
#define KOS_DISPLAY_ORIENTATION_180		2
#define KOS_DISPLAY_ORIENTATION_270		3

typedef struct {
    uint32_t w;
    uint32_t h;
    float xdpi;
    float ydpi;
    float fps;
    float density;
    uint8_t orientation;
    bool secure;
} KosDisplayInfo;

// Returned KosDisplayInfo's w/h is width/height when display's orientation is KOS_DISPLAY_ORIENTATION_0.
void kosGetDisplayInfo(KosDisplayInfo* info);
typedef void (*fdid_gui2_screen_captured)(uint8_t* pixel_buf, int length, int width, int height, uint32_t flags, void* user);
int kosRecordScreenLoop(uint32_t bitrate_kbps, uint32_t max_fps_to_encoder, uint8_t* pixel_buf, fdid_gui2_screen_captured did, void* user);
void kosStopRecordScreen();
void kosPauseRecordScreen(bool pause);
bool kosRecordScreenPaused();

#ifdef __cplusplus
}
#endif

#endif /* _LIBKOSAPI_GUI_H */
