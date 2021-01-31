#ifndef AIDL_GENERATED_ANDROID_HARDWARE_CAMERA2_I_CAMERA_DEVICE_CALLBACKS_H_
#define AIDL_GENERATED_ANDROID_HARDWARE_CAMERA2_I_CAMERA_DEVICE_CALLBACKS_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <camera/CameraMetadata.h>
#include <camera/CaptureResult.h>
#include <cstdint>
#include <utils/StrongPointer.h>

namespace android {

namespace hardware {

namespace camera2 {

class ICameraDeviceCallbacks : public ::android::IInterface {
public:
DECLARE_META_INTERFACE(CameraDeviceCallbacks);
enum  : int32_t {
  ERROR_CAMERA_INVALID_ERROR = -1,
  ERROR_CAMERA_DISCONNECTED = 0,
  ERROR_CAMERA_DEVICE = 1,
  ERROR_CAMERA_SERVICE = 2,
  ERROR_CAMERA_REQUEST = 3,
  ERROR_CAMERA_RESULT = 4,
  ERROR_CAMERA_BUFFER = 5,
};
virtual ::android::binder::Status onDeviceError(int32_t errorCode, const ::android::hardware::camera2::impl::CaptureResultExtras& resultExtras) = 0;
virtual ::android::binder::Status onDeviceIdle() = 0;
virtual ::android::binder::Status onCaptureStarted(const ::android::hardware::camera2::impl::CaptureResultExtras& resultExtras, int64_t timestamp) = 0;
virtual ::android::binder::Status onResultReceived(const ::android::hardware::camera2::impl::CameraMetadataNative& result, const ::android::hardware::camera2::impl::CaptureResultExtras& resultExtras) = 0;
virtual ::android::binder::Status onPrepared(int32_t streamId) = 0;
virtual ::android::binder::Status onRepeatingRequestError(int64_t lastFrameNumber) = 0;
enum Call {
  ONDEVICEERROR = ::android::IBinder::FIRST_CALL_TRANSACTION + 0,
  ONDEVICEIDLE = ::android::IBinder::FIRST_CALL_TRANSACTION + 1,
  ONCAPTURESTARTED = ::android::IBinder::FIRST_CALL_TRANSACTION + 2,
  ONRESULTRECEIVED = ::android::IBinder::FIRST_CALL_TRANSACTION + 3,
  ONPREPARED = ::android::IBinder::FIRST_CALL_TRANSACTION + 4,
  ONREPEATINGREQUESTERROR = ::android::IBinder::FIRST_CALL_TRANSACTION + 5,
};
};  // class ICameraDeviceCallbacks

}  // namespace camera2

}  // namespace hardware

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_HARDWARE_CAMERA2_I_CAMERA_DEVICE_CALLBACKS_H_