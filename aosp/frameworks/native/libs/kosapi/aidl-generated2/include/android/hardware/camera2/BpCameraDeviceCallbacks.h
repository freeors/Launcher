#ifndef AIDL_GENERATED_ANDROID_HARDWARE_CAMERA2_BP_CAMERA_DEVICE_CALLBACKS_H_
#define AIDL_GENERATED_ANDROID_HARDWARE_CAMERA2_BP_CAMERA_DEVICE_CALLBACKS_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <android/hardware/camera2/ICameraDeviceCallbacks.h>

namespace android {

namespace hardware {

namespace camera2 {

class BpCameraDeviceCallbacks : public ::android::BpInterface<ICameraDeviceCallbacks> {
public:
explicit BpCameraDeviceCallbacks(const ::android::sp<::android::IBinder>& _aidl_impl);
virtual ~BpCameraDeviceCallbacks() = default;
::android::binder::Status onDeviceError(int32_t errorCode, const ::android::hardware::camera2::impl::CaptureResultExtras& resultExtras) override;
::android::binder::Status onDeviceIdle() override;
::android::binder::Status onCaptureStarted(const ::android::hardware::camera2::impl::CaptureResultExtras& resultExtras, int64_t timestamp) override;
::android::binder::Status onResultReceived(const ::android::hardware::camera2::impl::CameraMetadataNative& result, const ::android::hardware::camera2::impl::CaptureResultExtras& resultExtras) override;
::android::binder::Status onPrepared(int32_t streamId) override;
::android::binder::Status onRepeatingRequestError(int64_t lastFrameNumber) override;
};  // class BpCameraDeviceCallbacks

}  // namespace camera2

}  // namespace hardware

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_HARDWARE_CAMERA2_BP_CAMERA_DEVICE_CALLBACKS_H_