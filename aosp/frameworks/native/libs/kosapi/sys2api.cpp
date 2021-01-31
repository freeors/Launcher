#define LOG_TAG "sys2api"

#include <kosapi/sys2.h>
#include <utils/Log.h>
#include <map>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include <kos/IAppManager.h>
#include <InputEventReceiver.h>

#include <rose/string_utils.hpp>


namespace android {

struct ClientAppInfo: public virtual android::RefBase
{
    ClientAppInfo(android::sp<android::IAppManager> appManager, int pid, const std::string& _package)
        : appManager(appManager)
        , pid(pid)
        , package(_package.empty()? "com.kos.launcher": _package)
        , did_touched(nullptr)
        , did_hover_moved(nullptr)
    {}

    virtual ~ClientAppInfo();

    android::sp<android::IAppManager> appManager;
    const int pid;
    const std::string package;
    android::sp<android::NativeInputEventReceiver> inputEventReceiver;
    fdid_sys2_touched did_touched;
    fdid_sys2_hover_moved did_hover_moved;
};

sp<IAppManager> get_appManager()
{
    sp<android::IBinder> binder = defaultServiceManager()->getService(String16(APP_MANAGER_SERVICE));
    android::sp<android::IAppManager> appManager = IAppManager::asInterface(binder);
    return appManager;
}

static android::sp<ClientAppInfo> app_internal;

ClientAppInfo* get_appinfo()
{
    if (app_internal.get() != nullptr) {
        return app_internal.get();
    }
    int pid = getpid();

    sp<IAppManager> appManager = get_appManager();
    const std::string package = appManager->getPackageName(pid).string();
    if (package.empty() || package.size() == 1) {
        return nullptr;
    }
    app_internal = new ClientAppInfo(appManager, pid, package);
    return app_internal.get();
}

ClientAppInfo::~ClientAppInfo()
{
    int pid = getpid();
    ALOGI("[sys2api.cpp] ClientAppInfo::~ClientAppInfo, E, pid: %i", pid);
    // sp<IAppManager> appManager = get_appManager();
    // appManager->appTerminated(pid);
    ALOGI("[sys2api.cpp] ClientAppInfo::~ClientAppInfo, X, pid: %i", pid);
}

}

NDK_EXPORT bool kosRunApp(const char* package)
{
    if (package == nullptr) {
        return false;
    }
    android::sp<android::IAppManager> appManager = android::get_appManager();

    int pid = appManager->runApp(android::String8(package));
    ALOGI("[sys2api.cpp] sys2_run_app, pid: %i, package: %s", pid, package);
    return pid > 0;
}

NDK_EXPORT void kosGetOsinfo(KosOsInfo* info)
{
    int pid = getpid();
    android::ClientAppInfo* app = android::get_appinfo();
    if (app == nullptr || pid != app->pid) {
        return;
    }

    ALOGI("[sys2api.cpp] sys2_get_osinfo, pid: %i, package: %s", pid, app->package.c_str());
    const std::string res_path = std::string("/data/app/") + app->package;
	SDL_strlcpy(info->res_path, res_path.c_str(), sizeof(info->res_path));

	const std::string userdata_path = std::string("/sdcard/Android/data/") + app->package + "/files";
	SDL_strlcpy(info->userdata_path, userdata_path.c_str(), sizeof(info->userdata_path));
}

NDK_EXPORT void kosSetSysDid(fdid_sys2_touched did_touched, fdid_sys2_hover_moved did_hover_moved)
{
    int pid = getpid();
    android::ClientAppInfo* app = android::get_appinfo();
    if (app == nullptr || pid != app->pid) {
         ALOGE("[sys2api.cpp] sys2_set_did, pid: %i, get_appinfo failed", getpid());
         return;
    }

    ALOGI("[sys2api.cpp] sys2_set_did, pid: %i", getpid());
    app->did_touched = did_touched;
    app->did_hover_moved = did_hover_moved;
    if (app->inputEventReceiver.get() != nullptr) {
        app->inputEventReceiver->set_did(did_touched, did_hover_moved);
    } else {
        ALOGI("[sys2api.cpp] sys2_set_did, pid: %i, inputEventReceiver is null", getpid());
    }
}

NDK_EXPORT void kosPumpEvent()
{
    const int pid = getpid();
    android::ClientAppInfo* app = android::get_appinfo();
    if (app == nullptr || pid != app->pid) {
        ALOGE("[sys2api.cpp] sys2_pump, fatal error, get_appinfo(%i) is null", pid);
        return;
    }

    if (app->inputEventReceiver.get() == nullptr) {
        android::sp<android::InputChannel> clientChannel = app->appManager->readClientChannel(pid);
        if (clientChannel.get() != nullptr) {
            ALOGD("[sys2api.cpp] sys2_pump, binder finished, fd: %i, name: %s", clientChannel->getFd(), clientChannel->getName().string());

            app->inputEventReceiver = new android::NativeInputEventReceiver(clientChannel);
            app->inputEventReceiver->initialize();
            app->inputEventReceiver->set_did(app->did_touched, app->did_hover_moved);

        } else {
            ALOGD("[sys2api.cpp] sys2_pump, readClientChannel(%i) return null", pid);
            return;
        }
    }
    if (app->inputEventReceiver.get() != nullptr) {
        app->inputEventReceiver->pollOnce();
    }
}