#ifndef GUI2MAIN_H_
#define GUI2MAIN_H_

#include <stdint.h>
#include <sys/types.h>

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

#include <androidfw/AssetManager.h>

#include <EGL/egl.h>
#include <GLES/gl.h>
#include <kosapi/gui.h>

namespace android {

class Surface;
class SurfaceComposerClient;
class SurfaceControl;

// ---------------------------------------------------------------------------

class gui2main: public IBinder::DeathRecipient
{
public:
    gui2main();
    virtual ~gui2main();

    sp<SurfaceComposerClient> session() const;
    ANativeWindow* get_surface();
    void release_surface();

    int native_window_get_width();
private:
    virtual void        onFirstRef();
    virtual void        binderDied(const wp<IBinder>& who);

private:
    sp<SurfaceComposerClient>       mSession;
    // int         mWidth;
    // int         mHeight;
    EGLDisplay  mDisplay;
    EGLDisplay  mContext;
    EGLDisplay  mSurface;
    sp<SurfaceControl> mFlingerSurfaceControl;
    sp<Surface> mFlingerSurface;
};

// ---------------------------------------------------------------------------

}; // namespace android

#endif