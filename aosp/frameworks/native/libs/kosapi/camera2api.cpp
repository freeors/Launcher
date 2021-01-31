#define LOG_TAG "camera2api"

#include <kosapi/camera.h>
#include <utils/Log.h>

#include <camera/Camera.h>
#include <binder/IMemory.h>

#include <binder/IServiceManager.h>

static android::sp<android::Camera> camera;

NDK_EXPORT int camera2_getNumberOfCameras()
{
	return 0;
}

NDK_EXPORT int camera2_getCameraInfo(int /*cameraId*/, char* /*info*/)
{
	return 0;
}

namespace android {

class tkOSCameraListener: public CameraListener
{
public:
    tkOSCameraListener()
        : did_camera2_frame_captured_(NULL)
        , user_(NULL)
    {}

    void set_did(fdid_camera2_frame_captured did, void* user)
    {
        did_camera2_frame_captured_ = did;
        user_ = user;
    }

    // void notify(int32_t msgType, int32_t ext1, int32_t ext2) override {};
    void notify(int32_t, int32_t, int32_t) override {};
    void postData(int32_t msgType, const sp<IMemory>& dataPtr, camera_frame_metadata_t *metadata);
    // void postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr) override {};
    void postDataTimestamp(nsecs_t, int32_t, const sp<IMemory>&) override {};
    // void postRecordingFrameHandleTimestamp(nsecs_t timestamp, native_handle_t* handle) override {};
    void postRecordingFrameHandleTimestamp(nsecs_t, native_handle_t*) override {};

private:
    fdid_camera2_frame_captured did_camera2_frame_captured_;
    void* user_;
};

void tkOSCameraListener::postData(int32_t /*msgType*/, const sp<IMemory>& dataPtr, camera_frame_metadata_t */*metadata*/)
{
    // ALOGD("tkOSCameraListener::postData, E");
    if (did_camera2_frame_captured_ == NULL) {
        return;
    }
    ssize_t offset;
    size_t size;
    sp<IMemoryHeap> heap = dataPtr->getMemory(&offset, &size);

    // ALOGD("tkOSCameraListener::postData: off=%zd, size=%zu", offset, size);
    uint8_t *heapBase = (uint8_t*)heap->base();

    if (heapBase != NULL) {
        did_camera2_frame_captured_(heapBase + offset, size, 1280, 720, 0, 0, user_);
    }
}

} //  namespace android

static android::sp<android::tkOSCameraListener> listener;

namespace android {

void notifyCameraService()
{
    sp<IServiceManager> sm = defaultServiceManager();
            sp<IBinder> binder = sm->getService(String16("media.camera"));
            sp<::android::hardware::ICameraService> camera_service = interface_cast<::android::hardware::ICameraService>(binder);
            std::vector<int32_t> newUserIds;
            newUserIds.push_back(0);
            camera_service->notifySystemEvent(::android::hardware::ICameraService::EVENT_USER_SWITCHED, newUserIds);
}

}

NDK_EXPORT int camera2_setup(int cameraId, const char* )
{

    ALOGI("[camera2_setup], notifyCameraService, pre");
    android::notifyCameraService();
    ALOGI("[camera2_setup], notifyCameraService, post");

	android::String16 clientName = android::String16("com.leagor.iaccess");

    // userid_t clientUserId = multiuser_get_user_id(clientUid);
	ALOGI("[camera2_setup], will call android::Camera::connect");
	camera = android::Camera::connect(cameraId, clientName, android::Camera::USE_CALLING_UID, android::Camera::USE_CALLING_PID);
    ALOGI("[camera2_setup], post android::Camera::connect, camera: %p", camera.get());
	if (camera == NULL) {
        return -EACCES;
    }

    listener = new android::tkOSCameraListener;
    camera->setListener(listener);
	return 0;
}

NDK_EXPORT void camera2_release(int /*cameraId*/)
{
	if (camera == NULL) {
	    return;
	}

	// tworker must be destroy in main thread. so it will throw exception.
	camera.clear();
}

NDK_EXPORT int camera2_startPreview(int /*cameraId*/, fdid_camera2_frame_captured did, void* user)
{
	if (camera == NULL) {
        return -EACCES;
    }

    listener->set_did(did, user);
    // camera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_CAMERA);
    camera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_ENABLE_MASK);
    if (camera->startPreview() != 0) {
        return -EACCES;
    }
	return 0;
}

NDK_EXPORT void camera2_stopPreview(int /*cameraId*/)
{
	if (camera == NULL) {
        return;
    }
	camera->stopPreview();
}