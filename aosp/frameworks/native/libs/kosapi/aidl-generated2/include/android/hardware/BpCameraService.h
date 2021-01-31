#ifndef AIDL_GENERATED_ANDROID_HARDWARE_BP_CAMERA_SERVICE_H_
#define AIDL_GENERATED_ANDROID_HARDWARE_BP_CAMERA_SERVICE_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <android/hardware/ICameraService.h>

namespace android {

namespace hardware {

class BpCameraService : public ::android::BpInterface<ICameraService> {
public:
explicit BpCameraService(const ::android::sp<::android::IBinder>& _aidl_impl);
virtual ~BpCameraService() = default;
::android::binder::Status getNumberOfCameras(int32_t type, int32_t* _aidl_return) override;
::android::binder::Status getCameraInfo(int32_t cameraId, ::android::hardware::CameraInfo* _aidl_return) override;
::android::binder::Status connect(const ::android::sp<::android::hardware::ICameraClient>& client, int32_t cameraId, const ::android::String16& opPackageName, int32_t clientUid, int32_t clientPid, ::android::sp<::android::hardware::ICamera>* _aidl_return) override;
::android::binder::Status connectDevice(const ::android::sp<::android::hardware::camera2::ICameraDeviceCallbacks>& callbacks, int32_t cameraId, const ::android::String16& opPackageName, int32_t clientUid, ::android::sp<::android::hardware::camera2::ICameraDeviceUser>* _aidl_return) override;
::android::binder::Status connectLegacy(const ::android::sp<::android::hardware::ICameraClient>& client, int32_t cameraId, int32_t halVersion, const ::android::String16& opPackageName, int32_t clientUid, ::android::sp<::android::hardware::ICamera>* _aidl_return) override;
::android::binder::Status addListener(const ::android::sp<::android::hardware::ICameraServiceListener>& listener) override;
::android::binder::Status removeListener(const ::android::sp<::android::hardware::ICameraServiceListener>& listener) override;
::android::binder::Status getCameraCharacteristics(int32_t cameraId, ::android::hardware::camera2::impl::CameraMetadataNative* _aidl_return) override;
::android::binder::Status getCameraVendorTagDescriptor(::android::hardware::camera2::params::VendorTagDescriptor* _aidl_return) override;
::android::binder::Status getLegacyParameters(int32_t cameraId, ::android::String16* _aidl_return) override;
::android::binder::Status supportsCameraApi(int32_t cameraId, int32_t apiVersion, bool* _aidl_return) override;
::android::binder::Status setTorchMode(const ::android::String16& CameraId, bool enabled, const ::android::sp<::android::IBinder>& clientBinder) override;
::android::binder::Status notifySystemEvent(int32_t eventId, const ::std::vector<int32_t>& args) override;
};  // class BpCameraService

}  // namespace hardware

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_HARDWARE_BP_CAMERA_SERVICE_H_