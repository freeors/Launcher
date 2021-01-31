LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

# setup for skia optimizations
#
ifneq ($(ARCH_ARM_HAVE_VFP),true)
    LOCAL_CFLAGS += -DSK_SOFTWARE_FLOAT
endif

ifeq ($(ARCH_ARM_HAVE_NEON),true)
    LOCAL_CFLAGS += -D__ARM_HAVE_NEON
endif

LOCAL_CFLAGS += -DWEBRTC_POSIX

# our source files
#
LOCAL_SRC_FILES:= \
    sendinput.cpp \
    NetdListener.cpp

LOCAL_SRC_FILES += \
    screenrecord/screenrecord.cpp \
    screenrecord/EglWindow.cpp \
    screenrecord/FrameOutput.cpp \
    screenrecord/Program.cpp

LOCAL_SRC_FILES += \
    webrtc/rtc_base/event.cpp \
    webrtc/rtc_base/checks.cpp

LOCAL_SHARED_LIBRARIES := \
    libcutils libutils libbinder libgui libinput \
    libEGL \
    libGLESv2 \
    libandroidfw \
    libcamera_client \
    libmedia \
    libstagefright \
    libstagefright_foundation \
    libcrypto \
    libsysutils

# libGLESv1_CM

LOCAL_C_INCLUDES += \
    frameworks/base/libs/hwui \
    frameworks/av/include/camera \
    system/media/camera/include \
    frameworks/native/include/media/openmax \
    frameworks/native/libs/kosapi/aidl-generated/include \
    frameworks/native/libs/kosapi/webrtc

LOCAL_MODULE:= libkosapi

LOCAL_CFLAGS += -fvisibility=hidden -DNDK_EXPORT='__attribute__((visibility ("default")))'
# LOCAL_CFLAGS += -Wall -Werror -Wunused -Wunreachable-code

LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES

# TODO: This is to work around b/24465209. Remove after root cause is fixed
LOCAL_LDFLAGS_arm := -Wl,--hash-style=both

include $(BUILD_SHARED_LIBRARY)

