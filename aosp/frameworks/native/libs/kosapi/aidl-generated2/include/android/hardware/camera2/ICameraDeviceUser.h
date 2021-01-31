#ifndef AIDL_GENERATED_ANDROID_HARDWARE_CAMERA2_I_CAMERA_DEVICE_USER_H_
#define AIDL_GENERATED_ANDROID_HARDWARE_CAMERA2_I_CAMERA_DEVICE_USER_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <camera/CameraMetadata.h>
#include <camera/camera2/CaptureRequest.h>
#include <camera/camera2/OutputConfiguration.h>
#include <camera/camera2/SubmitInfo.h>
#include <cstdint>
#include <gui/Surface.h>
#include <utils/StrongPointer.h>
#include <vector>

namespace android {

namespace hardware {

namespace camera2 {

class ICameraDeviceUser : public ::android::IInterface {
public:
DECLARE_META_INTERFACE(CameraDeviceUser);
enum  : int32_t {
  NO_IN_FLIGHT_REPEATING_FRAMES = -1,
  TEMPLATE_PREVIEW = 1,
  TEMPLATE_STILL_CAPTURE = 2,
  TEMPLATE_RECORD = 3,
  TEMPLATE_VIDEO_SNAPSHOT = 4,
  TEMPLATE_ZERO_SHUTTER_LAG = 5,
  TEMPLATE_MANUAL = 6,
};
virtual ::android::binder::Status disconnect() = 0;
virtual ::android::binder::Status submitRequest(const ::android::hardware::camera2::CaptureRequest& request, bool streaming, ::android::hardware::camera2::utils::SubmitInfo* _aidl_return) = 0;
virtual ::android::binder::Status submitRequestList(const ::std::vector<android::hardware::camera2::CaptureRequest>& requestList, bool streaming, ::android::hardware::camera2::utils::SubmitInfo* _aidl_return) = 0;
virtual ::android::binder::Status cancelRequest(int32_t requestId, int64_t* _aidl_return) = 0;
virtual ::android::binder::Status beginConfigure() = 0;
virtual ::android::binder::Status endConfigure(bool isConstrainedHighSpeed) = 0;
virtual ::android::binder::Status deleteStream(int32_t streamId) = 0;
virtual ::android::binder::Status createStream(const ::android::hardware::camera2::params::OutputConfiguration& outputConfiguration, int32_t* _aidl_return) = 0;
virtual ::android::binder::Status createInputStream(int32_t width, int32_t height, int32_t format, int32_t* _aidl_return) = 0;
virtual ::android::binder::Status getInputSurface(::android::view::Surface* _aidl_return) = 0;
virtual ::android::binder::Status createDefaultRequest(int32_t templateId, ::android::hardware::camera2::impl::CameraMetadataNative* _aidl_return) = 0;
virtual ::android::binder::Status getCameraInfo(::android::hardware::camera2::impl::CameraMetadataNative* _aidl_return) = 0;
virtual ::android::binder::Status waitUntilIdle() = 0;
virtual ::android::binder::Status flush(int64_t* _aidl_return) = 0;
virtual ::android::binder::Status prepare(int32_t streamId) = 0;
virtual ::android::binder::Status tearDown(int32_t streamId) = 0;
virtual ::android::binder::Status prepare2(int32_t maxCount, int32_t streamId) = 0;
virtual ::android::binder::Status setDeferredConfiguration(int32_t streamId, const ::android::hardware::camera2::params::OutputConfiguration& outputConfiguration) = 0;
enum Call {
  DISCONNECT = ::android::IBinder::FIRST_CALL_TRANSACTION + 0,
  SUBMITREQUEST = ::android::IBinder::FIRST_CALL_TRANSACTION + 1,
  SUBMITREQUESTLIST = ::android::IBinder::FIRST_CALL_TRANSACTION + 2,
  CANCELREQUEST = ::android::IBinder::FIRST_CALL_TRANSACTION + 3,
  BEGINCONFIGURE = ::android::IBinder::FIRST_CALL_TRANSACTION + 4,
  ENDCONFIGURE = ::android::IBinder::FIRST_CALL_TRANSACTION + 5,
  DELETESTREAM = ::android::IBinder::FIRST_CALL_TRANSACTION + 6,
  CREATESTREAM = ::android::IBinder::FIRST_CALL_TRANSACTION + 7,
  CREATEINPUTSTREAM = ::android::IBinder::FIRST_CALL_TRANSACTION + 8,
  GETINPUTSURFACE = ::android::IBinder::FIRST_CALL_TRANSACTION + 9,
  CREATEDEFAULTREQUEST = ::android::IBinder::FIRST_CALL_TRANSACTION + 10,
  GETCAMERAINFO = ::android::IBinder::FIRST_CALL_TRANSACTION + 11,
  WAITUNTILIDLE = ::android::IBinder::FIRST_CALL_TRANSACTION + 12,
  FLUSH = ::android::IBinder::FIRST_CALL_TRANSACTION + 13,
  PREPARE = ::android::IBinder::FIRST_CALL_TRANSACTION + 14,
  TEARDOWN = ::android::IBinder::FIRST_CALL_TRANSACTION + 15,
  PREPARE2 = ::android::IBinder::FIRST_CALL_TRANSACTION + 16,
  SETDEFERREDCONFIGURATION = ::android::IBinder::FIRST_CALL_TRANSACTION + 17,
};
};  // class ICameraDeviceUser

}  // namespace camera2

}  // namespace hardware

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_HARDWARE_CAMERA2_I_CAMERA_DEVICE_USER_H_