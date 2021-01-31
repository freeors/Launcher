#ifndef _LIBKOSAPI_MEDIACODEC_H
#define _LIBKOSAPI_MEDIACODEC_H

#ifndef _WIN32
#include <sys/cdefs.h>
#include <stdint.h>
#include <stdbool.h>
#else
#include <SDL_config.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Keep these in sync with their equivalents in <aosp>/frameworks/av/media/libstagefright/MediaDefs.cpp !!!
#define KOS_MIMETYPE_VIDEO_VP8	"video/x-vnd.on2.vp8"
#define KOS_MIMETYPE_VIDEO_VP9	"video/x-vnd.on2.vp9"
#define KOS_MIMETYPE_VIDEO_AVC	"video/avc"
#define KOS_MIMETYPE_VIDEO_HEVC	"video/hevc"

enum {
	DEQUEUE_INFO_TRY_AGAIN_LATER         = -1,
	DEQUEUE_INFO_OUTPUT_FORMAT_CHANGED   = -2,
	DEQUEUE_INFO_OUTPUT_BUFFERS_CHANGED  = -3,
	DEQUEUE_INFO_OTHER_ERROR			 = -4, // include other error
};

#define KOS_MAX_NAME_LEN	48
#define KOS_MAX_CAP_SIZE	4 // Usually one MediaCodecInfo has one Capabilities, but there may be more than one.
#define KOS_MAX_COLORFORMAT	8
typedef struct {
    char	mime[KOS_MAX_NAME_LEN];
	int		colorFormats[KOS_MAX_COLORFORMAT];
} kosMediaCodecCap;

// Media codec info, see MediaCodecList_getCodecInfoAt()
typedef struct {
    char		name[KOS_MAX_NAME_LEN];
	bool		isEncoder;
	kosMediaCodecCap caps[KOS_MAX_CAP_SIZE];
} kosMediaCodecInfo;

int MediaCodecList_getCodecCount();
void MediaCodecList_getCodecInfoAt(int index, kosMediaCodecInfo* info);

//
// Media Codec session
//
typedef struct {
	char	mime[KOS_MAX_NAME_LEN];
	int		width;
	int		height;
	int		colorFormat;
	int		stride;
	int		sliceHeight;

	int		bitrate;
	int		bitrateMode;
	int		frameRate;
	int		iFrameIntervalSec;
	int		profile;
	int		level;
} kosMediaFormat;
kosMediaFormat MediaFormat_create(const char* mime, int width, int height);
int MediaCodec_setup(const char* name);

#define KOS_MEDIACODEC_CONFIGURE_FLAG_DECODE	0
#define KOS_MEDIACODEC_CONFIGURE_FLAG_ENCODE	1
int MediaCodec_configure(int handle, kosMediaFormat* format, int flags);
void MediaCodec_start(int handle);
void MediaCodec_stop(int handle);
void MediaCodec_flush(int handle);
void MediaCodec_release(int handle);

// Media codec info, see MediaCodecList_getCodecInfoAt()
typedef struct {
    uint8_t *mData;
    size_t mCapacity;
} kosABuffer;
int MediaCodec_getBufferLength(int handle, bool input);
bool MediaCodec_getBuffer(int handle, bool input, int index, kosABuffer* buffer);
int MediaCodec_dequeueInputBuffer(int handle, int64_t timeoutUs);
bool MediaCodec_queueInputBuffer(int handle, int index, size_t offset, size_t size, int64_t presentationTimeStamUs);

enum {
	MediaCodec_BUFFER_FLAG_KEY_FRAME	= 1,
	MediaCodec_BUFFER_FLAG_CODEC_CONFIG	= 2,
};

typedef struct {
	/**
    * The start-offset of the data in the buffer.
    */
	int offset;

	/**
    * The amount of data (in bytes) in the buffer.  If this is {@code 0},
    * the buffer has no data in it and can be discarded.  The only
    * use of a 0-size buffer is to carry the end-of-stream marker.
    */
	int size;

	/**
    * The presentation timestamp in microseconds for the buffer.
    * This is derived from the presentation timestamp passed in
    * with the corresponding input buffer.  This should be ignored for
    * a 0-sized buffer.
    */
	int64_t presentationTimeUs;

	/**
    * Buffer flags associated with the buffer.  A combination of
    * {@link #BUFFER_FLAG_KEY_FRAME} and {@link #BUFFER_FLAG_END_OF_STREAM}.
    *
    * <p>Encoded buffers that are key frames are marked with
    * {@link #BUFFER_FLAG_KEY_FRAME}.
    *
    * <p>The last output buffer corresponding to the input buffer
    * marked with {@link #BUFFER_FLAG_END_OF_STREAM} will also be marked
    * with {@link #BUFFER_FLAG_END_OF_STREAM}. In some cases this could
    * be an empty buffer, whose sole purpose is to carry the end-of-stream
    * marker.
    */
	int flags;
} kosBufferInfo;
int MediaCodec_dequeueOutputBuffer(int handle, int64_t timeoutUs, kosBufferInfo* bufferInfo);
void MediaCodec_releaseOutputBuffer(int handle, int index);
bool MediaCodec_getOutputFormat(int handle, kosMediaFormat* format);

// Type of bitrate adjustment for video encoder.
enum BitrateAdjustmentType {
	// No adjustment - video encoder has no known bitrate problem.
	NO_ADJUSTMENT,
	// Framerate based bitrate adjustment is required - HW encoder does not use frame
	// timestamps to calculate frame bitrate budget and instead is relying on initial
	// fps configuration assuming that all frames are coming at fixed initial frame rate.
	FRAMERATE_ADJUSTMENT,
	// Dynamic bitrate adjustment is required - HW encoder used frame timestamps, but actual
	// bitrate deviates too much from the target value.
	DYNAMIC_ADJUSTMENT
};

enum {
	KOS_PARAMETER_FLAG_REQUEST_SYNC_FRAME	= 0x1,
	KOS_PARAMETER_FLAG_VIDEO_BITRATE		= 0x2,
};

typedef struct {
	uint32_t flags;
	int		request_sync_frame;
	int		video_bitrate;
} kosMediaCodecParameters;
kosMediaCodecParameters MediaCodec_createParameters(uint32_t flags);
void MediaCodec_setParameters(int handle, kosMediaCodecParameters* params);


#ifdef __cplusplus
}
#endif

#endif /* _LIBKOSAPI_MEDIACODEC_H */
