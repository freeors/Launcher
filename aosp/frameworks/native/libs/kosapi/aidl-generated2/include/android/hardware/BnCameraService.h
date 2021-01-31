#ifndef AIDL_GENERATED_ANDROID_HARDWARE_BN_CAMERA_SERVICE_H_
#define AIDL_GENERATED_ANDROID_HARDWARE_BN_CAMERA_SERVICE_H_

#include <binder/IInterface.h>
#include <android/hardware/ICameraService.h>

namespace android {

namespace hardware {

class BnCameraService : public ::android::BnInterface<ICameraService> {
public:
::android::status_t onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags = 0) override;
};  // class BnCameraService

}  // namespace hardware

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_HARDWARE_BN_CAMERA_SERVICE_H_