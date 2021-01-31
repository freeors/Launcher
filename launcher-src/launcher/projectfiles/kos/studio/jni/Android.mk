LOCAL_PATH := $(call my-dir)/../../../..

include $(CLEAR_VARS)

LOCAL_MODULE := main

LOCAL_CFLAGS := -DWEBRTC_LINUX -DWEBRTC_ANDROID -DWEBRTC_POSIX -DWEBRTC_NS_FIXED -DWEBRTC_APM_DEBUG_DUMP=0 -DWEBRTC_INTELLIGIBILITY_ENHANCER=0 \
-DWEBRTC_CODEC_ISACFX -DHAVE_SCTP -DHAVE_WEBRTC_VIDEO -DHAVE_WEBRTC_VOICE \
-DWEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE -DWEBRTC_USE_BUILTIN_ISAC_FIX=1 -DWEBRTC_USE_BUILTIN_ISAC_FLOAT=0 -DBORINGSSL_IMPLEMENTATION -DBORINGSSL_NO_STATIC_INITIALIZER \
-DOPENSSL_SMALL -DBORINGSSL_ALLOW_CXX_RUNTIME -DWEBRTC_THREAD_RR -DWEBRTC_OPUS_SUPPORT_120MS_PTIME=1 \
-DNO_TCMALLOC

LOCAL_CFLAGS += -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -DWEBRTC_HAS_NEON -DWEBRTC_ARCH_ARM -DWEBRTC_ARCH_ARM_V7
LOCAL_CFLAGS += -DWEBRTC_BUILD_LIBEVENT
LOCAL_CFLAGS += -mfpu=neon -O3
LOCAL_CFLAGS += -D_KOS

LOCAL_CPPFLAGS += -Wno-inconsistent-missing-override -Wno-nonportable-include-path -Wno-narrowing -DGOOGLE_PROTOBUF_DONT_USE_UNALIGNED
LOCAL_CPPFLAGS += -std=c++1y

LOCAL_CPP_EXTENSION := .cxx .cpp .cc

LOCAL_C_INCLUDES := $(LOCAL_PATH)/external \
	$(LOCAL_PATH)/external/expat \
	$(LOCAL_PATH)/external/boost \
	$(LOCAL_PATH)/external/bzip2 \
	$(LOCAL_PATH)/external/zlib \
	$(LOCAL_PATH)/external/boringssl/include \
	$(LOCAL_PATH)/external/chromium \
	$(LOCAL_PATH)/external/expat/lib \
	$(LOCAL_PATH)/external/live555 \
	$(LOCAL_PATH)/external/usrsctplib \
	$(LOCAL_PATH)/external/third_party/libyuv/include \
	$(LOCAL_PATH)/external/third_party/libsrtp/include \
	$(LOCAL_PATH)/external/third_party/libsrtp/crypto/include \
	$(LOCAL_PATH)/external/base/third_party/libevent/android \
	$(LOCAL_PATH)/external/webrtc \
	$(LOCAL_PATH)/external/webrtc/common_audio/signal_processing/include \
	$(LOCAL_PATH)/external/webrtc/modules/audio_coding/codecs/isac/main/include \
	$(LOCAL_PATH)/../linker/include/SDL2 \
	$(LOCAL_PATH)/../linker/include/SDL2_image \
	$(LOCAL_PATH)/../linker/include/SDL2_mixer \
	$(LOCAL_PATH)/../linker/include/SDL2_ttf \
	$(LOCAL_PATH)/../linker/include/libvpx \
	$(LOCAL_PATH)/../linker/include/opencv \
	$(LOCAL_PATH)/../linker/include/kos \
	$(LOCAL_PATH)/external/protobuf/src \
	$(LOCAL_PATH)/external/tensorflow \
	$(LOCAL_PATH)/external/tensorflow/tensorflow/lite/downloads/eigen \
	$(LOCAL_PATH)/external/tensorflow/tensorflow/lite/downloads/gemmlowp \
	$(LOCAL_PATH)/external/tensorflow/tensorflow/lite/downloads/farmhash/src \
	$(LOCAL_PATH)/external/tensorflow/tensorflow/lite/downloads/flatbuffers/include \
	$(LOCAL_PATH)/external/zxing \
	$(LOCAL_PATH)/librose \
	$(LOCAL_PATH)/studio

LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/studio/*.c) \
	$(wildcard $(LOCAL_PATH)/studio/*.cc) \
	$(wildcard $(LOCAL_PATH)/studio/*.cpp) \
	$(wildcard $(LOCAL_PATH)/studio/gui/dialogs/*.c) \
	$(wildcard $(LOCAL_PATH)/studio/gui/dialogs/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/event/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/event/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/iterator/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/iterator/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/widget_Definition/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/widget_Definition/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/window_Builder/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/window_Builder/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/gui/dialogs/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/gui/dialogs/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/gui/lib/types/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/gui/lib/types/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/gui/widgets/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/gui/widgets/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/minizip/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/minizip/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/ocr/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/ocr/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/plot/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/plot/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/serialization/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/serialization/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/boost/libs/iostreams/src/*.c) \
	$(wildcard $(LOCAL_PATH)/external/boost/libs/iostreams/src/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/boost/libs/regex/src/*.c) \
	$(wildcard $(LOCAL_PATH)/external/boost/libs/regex/src/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/gettext/gettext-runtime/intl/*.c) \
	$(wildcard $(LOCAL_PATH)/external/gettext/gettext-runtime/intl/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/libiconv/lib/*.c) \
	$(wildcard $(LOCAL_PATH)/external/libiconv/lib/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/zxing/aztec/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/zxing/datamatrix/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/zxing/maxicode/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/zxing/oned/rss/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/zxing/oned/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/zxing/pdf417/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/zxing/qrcode/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/zxing/textcodec/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/zxing/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/bzip2/*.c) \
	$(wildcard $(LOCAL_PATH)/external/bzip2/*.cpp))


include $(LOCAL_PATH)/external/boringssl/Android.mk
include $(LOCAL_PATH)/external/chromium/Android-kos.mk
include $(LOCAL_PATH)/external/expat/Android.mk
include $(LOCAL_PATH)/external/live555/Android.mk
include $(LOCAL_PATH)/external/usrsctplib/Android.mk
include $(LOCAL_PATH)/external/base/third_party/libevent/Android.mk
include $(LOCAL_PATH)/external/third_party/libyuv/Android.mk
include $(LOCAL_PATH)/external/third_party/libsrtp/Android.mk
include $(LOCAL_PATH)/external/tensorflow/Android.mk

WEBRTC_SUBPATH := external/webrtc
include $(LOCAL_PATH)/$(WEBRTC_SUBPATH)/Android-kos.mk

LOCAL_LDFLAGS += -L$(LOCAL_PATH)/../linker/android/lib/armeabi-v7a -L$(LOCAL_PATH)/../linker/android/lib-kos-private/armeabi-v7a
LOCAL_LDLIBS := -lz -llog -lOpenSLES -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lvpx -lprotobuf -lopencv -lkosapi
LOCAL_SHORT_COMMANDS := true

include $(BUILD_SHARED_LIBRARY)