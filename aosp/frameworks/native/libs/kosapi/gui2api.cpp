#define LOG_TAG "gui2api"

#include <kosapi/gui.h>
#include <utils/Log.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include "gui2main.h"

android::sp<android::gui2main> gui2;

NDK_EXPORT ANativeWindow* gui2_get_surface()
{
	ALOGI("gui2_get_surface, E");

    gui2 = new android::gui2main;

    ALOGI("gui2_get_surface, will get_surface, gui2.getStrongCount: %i", gui2->getStrongCount());
    ANativeWindow* native_window = gui2->get_surface();

    ALOGI("gui2_get_surface, X, gui2.getStrongCount: %i, ANativeWindow: %p", gui2->getStrongCount(), native_window);
	return native_window;
}

NDK_EXPORT int gui2_natvie_window_get_width(ANativeWindow* )
{
    ALOGI("gui2_native_window_get_width, E");
    gui2->native_window_get_width();
    ALOGI("gui2_native_window_get_width, X");
    return 0;
}

NDK_EXPORT int gui2_window_get_height(ANativeWindow* )
{
    ALOGI("gui2_native_window_get_height, E");

    ALOGI("gui2_native_window_get_height, X");
    return 0;
}