#ifndef AIDL_GENERATED_ANDROID_HARDWARE_BP_CAMERA_SERVICE_LISTENER_H_
#define AIDL_GENERATED_ANDROID_HARDWARE_BP_CAMERA_SERVICE_LISTENER_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <android/hardware/ICameraServiceListener.h>

namespace android {

namespace hardware {

class BpCameraServiceListener : public ::android::BpInterface<ICameraServiceListener> {
public:
explicit BpCameraServiceListener(const ::android::sp<::android::IBinder>& _aidl_impl);
virtual ~BpCameraServiceListener() = default;
::android::binder::Status onStatusChanged(int32_t status, int32_t cameraId) override;
::android::binder::Status onTorchStatusChanged(int32_t status, const ::android::String16& cameraId) override;
};  // class BpCameraServiceListener

}  // namespace hardware

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_HARDWARE_BP_CAMERA_SERVICE_LISTENER_H_