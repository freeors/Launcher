#ifndef AIDL_GENERATED_ANDROID_HARDWARE_BN_CAMERA_SERVICE_LISTENER_H_
#define AIDL_GENERATED_ANDROID_HARDWARE_BN_CAMERA_SERVICE_LISTENER_H_

#include <binder/IInterface.h>
#include <android/hardware/ICameraServiceListener.h>

namespace android {

namespace hardware {

class BnCameraServiceListener : public ::android::BnInterface<ICameraServiceListener> {
public:
::android::status_t onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags = 0) override;
};  // class BnCameraServiceListener

}  // namespace hardware

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_HARDWARE_BN_CAMERA_SERVICE_LISTENER_H_