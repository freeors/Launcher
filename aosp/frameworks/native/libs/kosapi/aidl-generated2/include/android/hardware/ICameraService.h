#ifndef AIDL_GENERATED_ANDROID_HARDWARE_I_CAMERA_SERVICE_H_
#define AIDL_GENERATED_ANDROID_HARDWARE_I_CAMERA_SERVICE_H_

#include <android/hardware/ICamera.h>
#include <android/hardware/ICameraClient.h>
#include <android/hardware/ICameraServiceListener.h>
#include <android/hardware/camera2/ICameraDeviceCallbacks.h>
#include <android/hardware/camera2/ICameraDeviceUser.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <camera/CameraBase.h>
#include <camera/CameraMetadata.h>
#include <camera/VendorTagDescriptor.h>
#include <cstdint>
#include <utils/String16.h>
#include <utils/StrongPointer.h>
#include <vector>

namespace android {

namespace hardware {

class ICameraService : public ::android::IInterface {
public:
DECLARE_META_INTERFACE(CameraService);
enum  : int32_t {
  ERROR_PERMISSION_DENIED = 1,
  ERROR_ALREADY_EXISTS = 2,
  ERROR_ILLEGAL_ARGUMENT = 3,
  ERROR_DISCONNECTED = 4,
  ERROR_TIMED_OUT = 5,
  ERROR_DISABLED = 6,
  ERROR_CAMERA_IN_USE = 7,
  ERROR_MAX_CAMERAS_IN_USE = 8,
  ERROR_DEPRECATED_HAL = 9,
  ERROR_INVALID_OPERATION = 10,
  CAMERA_TYPE_BACKWARD_COMPATIBLE = 0,
  CAMERA_TYPE_ALL = 1,
  USE_CALLING_UID = -1,
  USE_CALLING_PID = -1,
  CAMERA_HAL_API_VERSION_UNSPECIFIED = -1,
  API_VERSION_1 = 1,
  API_VERSION_2 = 2,
  EVENT_NONE = 0,
  EVENT_USER_SWITCHED = 1,
};
virtual ::android::binder::Status getNumberOfCameras(int32_t type, int32_t* _aidl_return) = 0;
virtual ::android::binder::Status getCameraInfo(int32_t cameraId, ::android::hardware::CameraInfo* _aidl_return) = 0;
virtual ::android::binder::Status connect(const ::android::sp<::android::hardware::ICameraClient>& client, int32_t cameraId, const ::android::String16& opPackageName, int32_t clientUid, int32_t clientPid, ::android::sp<::android::hardware::ICamera>* _aidl_return) = 0;
virtual ::android::binder::Status connectDevice(const ::android::sp<::android::hardware::camera2::ICameraDeviceCallbacks>& callbacks, int32_t cameraId, const ::android::String16& opPackageName, int32_t clientUid, ::android::sp<::android::hardware::camera2::ICameraDeviceUser>* _aidl_return) = 0;
virtual ::android::binder::Status connectLegacy(const ::android::sp<::android::hardware::ICameraClient>& client, int32_t cameraId, int32_t halVersion, const ::android::String16& opPackageName, int32_t clientUid, ::android::sp<::android::hardware::ICamera>* _aidl_return) = 0;
virtual ::android::binder::Status addListener(const ::android::sp<::android::hardware::ICameraServiceListener>& listener) = 0;
virtual ::android::binder::Status removeListener(const ::android::sp<::android::hardware::ICameraServiceListener>& listener) = 0;
virtual ::android::binder::Status getCameraCharacteristics(int32_t cameraId, ::android::hardware::camera2::impl::CameraMetadataNative* _aidl_return) = 0;
virtual ::android::binder::Status getCameraVendorTagDescriptor(::android::hardware::camera2::params::VendorTagDescriptor* _aidl_return) = 0;
virtual ::android::binder::Status getLegacyParameters(int32_t cameraId, ::android::String16* _aidl_return) = 0;
virtual ::android::binder::Status supportsCameraApi(int32_t cameraId, int32_t apiVersion, bool* _aidl_return) = 0;
virtual ::android::binder::Status setTorchMode(const ::android::String16& CameraId, bool enabled, const ::android::sp<::android::IBinder>& clientBinder) = 0;
virtual ::android::binder::Status notifySystemEvent(int32_t eventId, const ::std::vector<int32_t>& args) = 0;
enum Call {
  GETNUMBEROFCAMERAS = ::android::IBinder::FIRST_CALL_TRANSACTION + 0,
  GETCAMERAINFO = ::android::IBinder::FIRST_CALL_TRANSACTION + 1,
  CONNECT = ::android::IBinder::FIRST_CALL_TRANSACTION + 2,
  CONNECTDEVICE = ::android::IBinder::FIRST_CALL_TRANSACTION + 3,
  CONNECTLEGACY = ::android::IBinder::FIRST_CALL_TRANSACTION + 4,
  ADDLISTENER = ::android::IBinder::FIRST_CALL_TRANSACTION + 5,
  REMOVELISTENER = ::android::IBinder::FIRST_CALL_TRANSACTION + 6,
  GETCAMERACHARACTERISTICS = ::android::IBinder::FIRST_CALL_TRANSACTION + 7,
  GETCAMERAVENDORTAGDESCRIPTOR = ::android::IBinder::FIRST_CALL_TRANSACTION + 8,
  GETLEGACYPARAMETERS = ::android::IBinder::FIRST_CALL_TRANSACTION + 9,
  SUPPORTSCAMERAAPI = ::android::IBinder::FIRST_CALL_TRANSACTION + 10,
  SETTORCHMODE = ::android::IBinder::FIRST_CALL_TRANSACTION + 11,
  NOTIFYSYSTEMEVENT = ::android::IBinder::FIRST_CALL_TRANSACTION + 12,
};
};  // class ICameraService

}  // namespace hardware

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_HARDWARE_I_CAMERA_SERVICE_H_