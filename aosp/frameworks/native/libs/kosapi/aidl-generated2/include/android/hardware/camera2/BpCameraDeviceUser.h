#ifndef AIDL_GENERATED_ANDROID_HARDWARE_CAMERA2_BP_CAMERA_DEVICE_USER_H_
#define AIDL_GENERATED_ANDROID_HARDWARE_CAMERA2_BP_CAMERA_DEVICE_USER_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <android/hardware/camera2/ICameraDeviceUser.h>

namespace android {

namespace hardware {

namespace camera2 {

class BpCameraDeviceUser : public ::android::BpInterface<ICameraDeviceUser> {
public:
explicit BpCameraDeviceUser(const ::android::sp<::android::IBinder>& _aidl_impl);
virtual ~BpCameraDeviceUser() = default;
::android::binder::Status disconnect() override;
::android::binder::Status submitRequest(const ::android::hardware::camera2::CaptureRequest& request, bool streaming, ::android::hardware::camera2::utils::SubmitInfo* _aidl_return) override;
::android::binder::Status submitRequestList(const ::std::vector<android::hardware::camera2::CaptureRequest>& requestList, bool streaming, ::android::hardware::camera2::utils::SubmitInfo* _aidl_return) override;
::android::binder::Status cancelRequest(int32_t requestId, int64_t* _aidl_return) override;
::android::binder::Status beginConfigure() override;
::android::binder::Status endConfigure(bool isConstrainedHighSpeed) override;
::android::binder::Status deleteStream(int32_t streamId) override;
::android::binder::Status createStream(const ::android::hardware::camera2::params::OutputConfiguration& outputConfiguration, int32_t* _aidl_return) override;
::android::binder::Status createInputStream(int32_t width, int32_t height, int32_t format, int32_t* _aidl_return) override;
::android::binder::Status getInputSurface(::android::view::Surface* _aidl_return) override;
::android::binder::Status createDefaultRequest(int32_t templateId, ::android::hardware::camera2::impl::CameraMetadataNative* _aidl_return) override;
::android::binder::Status getCameraInfo(::android::hardware::camera2::impl::CameraMetadataNative* _aidl_return) override;
::android::binder::Status waitUntilIdle() override;
::android::binder::Status flush(int64_t* _aidl_return) override;
::android::binder::Status prepare(int32_t streamId) override;
::android::binder::Status tearDown(int32_t streamId) override;
::android::binder::Status prepare2(int32_t maxCount, int32_t streamId) override;
::android::binder::Status setDeferredConfiguration(int32_t streamId, const ::android::hardware::camera2::params::OutputConfiguration& outputConfiguration) override;
};  // class BpCameraDeviceUser

}  // namespace camera2

}  // namespace hardware

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_HARDWARE_CAMERA2_BP_CAMERA_DEVICE_USER_H_