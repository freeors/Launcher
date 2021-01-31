#ifndef _LIBKOSAPI_CAMERA_H
#define _LIBKOSAPI_CAMERA_H

#ifndef _WIN32
#include <sys/cdefs.h>
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

int kosGetNumberOfCameras();
int kosGetCameraInfo(int cameraId, char* info);
int kosSetupCamera(int cameraId, const char* clientPackageName);
void kosReleaseCamera(int cameraId);

typedef void (*fdid_camera2_frame_captured)(const void* data, int length, int width, int height, int rotation, int64_t timestamp_ns, void* user);
int kosStartPreviewCamera(int cameraId, fdid_camera2_frame_captured did, void* user);
void kosStopPreviewCamera(int cameraId);

#ifdef __cplusplus
}
#endif

#endif /* _LIBKOSAPI_CAMERA_H */
