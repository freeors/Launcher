#define LOG_TAG "gui2main"

#include <utils/Log.h>
#include "gui2main.h"

#include <ui/DisplayInfo.h>

#include <gui/ISurfaceComposer.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
/*
// TODO: Fix Skia.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <SkBitmap.h>
#include <SkStream.h>
#include <SkImageDecoder.h>
#pragma GCC diagnostic pop
*/
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/eglext.h>

namespace android {

gui2main::gui2main()
{
    ALOGI("gui2main::gui2main, E");
    mSession = new SurfaceComposerClient();
    ALOGI("gui2main::gui2main, X");
}

gui2main::~gui2main()
{
    ALOGI("gui2main::~gui2main, E");
    release_surface();
    ALOGI("gui2main::~gui2main, X");
}

void gui2main::onFirstRef() {
    status_t err = mSession->linkToComposerDeath(this);
    ALOGE_IF(err, "linkToComposerDeath failed (%s) ", strerror(-err));
/*
    if (err == NO_ERROR) {
        run("BootAnimation", PRIORITY_DISPLAY);
    }
*/
}

sp<SurfaceComposerClient> gui2main::session() const {
    return mSession;
}


void gui2main::binderDied(const wp<IBinder>&)
{
    // woah, surfaceflinger died!
    ALOGD("SurfaceFlinger died, exiting...");

    // calling requestExit() is not enough here because the Surface code
    // might be blocked on a condition variable that will never be updated.
    kill( getpid(), SIGKILL );
    // requestExit();
    // audioplay::destroy();
}

void gui2main::release_surface()
{
    ALOGI("gui2main::release_surface, E");
    if (mFlingerSurface.get() == NULL) {
        ALOGI("gui2main::release_surface, E, surface is null, do nothing");
        return;
    }
    eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(mDisplay, mContext);
    eglDestroySurface(mDisplay, mSurface);
    mFlingerSurface.clear();
    mFlingerSurfaceControl.clear();
    eglTerminate(mDisplay);
    ALOGI("gui2main::release_surface, X");
}

ANativeWindow* gui2main::get_surface()
{
    ALOGI("gui2main::get_surface, E");
    if (mFlingerSurface.get()) {
        ALOGI("gui2main::get_surface, X, surface has created, %p", mFlingerSurface.get());
        return mFlingerSurface.get();
    }
    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(
            ISurfaceComposer::eDisplayIdMain));
    DisplayInfo dinfo;
    status_t status = SurfaceComposerClient::getDisplayInfo(dtoken, &dinfo);
    if (status) {
        ALOGI("gui2main::get_surface, X, getDisplayInfo faile, status: %i", status);
        return NULL;
    }

    ALOGI("getDisplayInfo, info: (%i x %i)", dinfo.w, dinfo.h);

    // create the native surface
    int curWidth = dinfo.w;
    int curHeight = dinfo.h;
/*
    if(mShutdown && mReverseAxis){
        curWidth = dinfo.h;
        curHeight = dinfo.w;
    }
*/
    sp<SurfaceControl> control = session()->createSurface(String8("sdlgui2"),
            curWidth, curHeight, PIXEL_FORMAT_RGB_565);

    ALOGI("session()->createSurface, control: %p", control.get());
    SurfaceComposerClient::openGlobalTransaction();
    control->setLayer(0x40000000);
    SurfaceComposerClient::closeGlobalTransaction();

    sp<Surface> s = control->getSurface();

    ANativeWindow* nw = s.get();
    ALOGI("control->getSurface, Surface: %p, nw: %p", s.get(), nw);
    // output example: control->getSurface, Surface: 0x91eac800, nw: 0x91eac808
    // shoudle use s.get() valuate nw, must not use nw = reinterpret_cast<ANativeWindow*>(s.get()), they are different!

/*
    // initialize opengl and egl
    const EGLint attribs[] = {
            EGL_RED_SIZE,   8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE,  8,
            EGL_DEPTH_SIZE, 0,
            EGL_NONE
    };
    EGLint w, h;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    ALOGI("eglGetDisplay");
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    ALOGI("eglInitialize");
    eglInitialize(display, 0, 0);

    ALOGI("eglChooseConfig");
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    ALOGI("eglCreateWindowSurface");
    surface = eglCreateWindowSurface(display, config, s.get(), NULL);
    ALOGI("eglCreateContext");
    context = eglCreateContext(display, config, NULL, NULL);
    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);
    ALOGI("eglQuerySurface, EGL_WIDTH: %i, EGL_HEIGHT: %i", w, h);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        ALOGI("gui2main::get_surface, X, eglMakeCurrent fail");
        return s.get();
    }

    mDisplay = display;
    mContext = context;
    mSurface = surface;
    mWidth = w;
    mHeight = h;
*/
    mFlingerSurfaceControl = control;
    mFlingerSurface = s;


    return nw;
}

int gui2main::native_window_get_width()
{
    ALOGI("gui2main::native_window_get_width, E");
    // initialize opengl and egl
    const EGLint attribs[] = {
            EGL_RED_SIZE,   8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE,  8,
            EGL_DEPTH_SIZE, 0,
            EGL_NONE
    };
    EGLint w, h;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    ALOGI("eglGetDisplay");
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    ALOGI("eglInitialize, display: %p", display);
    eglInitialize(display, 0, 0);

    ALOGI("eglChooseConfig");
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    ALOGI("eglCreateWindowSurface");
    surface = eglCreateWindowSurface(display, config, mFlingerSurface.get(), NULL);
    ALOGI("eglCreateContext");
    context = eglCreateContext(display, config, NULL, NULL);
    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);
    ALOGI("eglQuerySurface, EGL_WIDTH: %i, EGL_HEIGHT: %i", w, h);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        ALOGI("gui2main::get_surface, X, eglMakeCurrent fail");
        return 0;
    }
    ALOGI("gui2main::native_window_get_width, X");
    return 0;
}


}; // namespace android