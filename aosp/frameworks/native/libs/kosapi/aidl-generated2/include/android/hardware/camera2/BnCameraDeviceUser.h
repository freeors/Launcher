#ifndef AIDL_GENERATED_ANDROID_HARDWARE_CAMERA2_BN_CAMERA_DEVICE_USER_H_
#define AIDL_GENERATED_ANDROID_HARDWARE_CAMERA2_BN_CAMERA_DEVICE_USER_H_

#include <binder/IInterface.h>
#include <android/hardware/camera2/ICameraDeviceUser.h>

namespace android {

namespace hardware {

namespace camera2 {

class BnCameraDeviceUser : public ::android::BnInterface<ICameraDeviceUser> {
public:
::android::status_t onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags = 0) override;
};  // class BnCameraDeviceUser

}  // namespace camera2

}  // namespace hardware

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_HARDWARE_CAMERA2_BN_CAMERA_DEVICE_USER_H_