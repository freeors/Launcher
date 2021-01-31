#ifndef AIDL_GENERATED_ANDROID_HARDWARE_CAMERA2_BN_CAMERA_DEVICE_CALLBACKS_H_
#define AIDL_GENERATED_ANDROID_HARDWARE_CAMERA2_BN_CAMERA_DEVICE_CALLBACKS_H_

#include <binder/IInterface.h>
#include <android/hardware/camera2/ICameraDeviceCallbacks.h>

namespace android {

namespace hardware {

namespace camera2 {

class BnCameraDeviceCallbacks : public ::android::BnInterface<ICameraDeviceCallbacks> {
public:
::android::status_t onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags = 0) override;
};  // class BnCameraDeviceCallbacks

}  // namespace camera2

}  // namespace hardware

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_HARDWARE_CAMERA2_BN_CAMERA_DEVICE_CALLBACKS_H_