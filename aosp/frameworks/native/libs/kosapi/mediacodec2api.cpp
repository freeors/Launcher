#define LOG_TAG "mediacodec2api"

#include <kosapi/mediacodec.h>

#include <media/stagefright/MediaCodecList.h>
#include <media/IMediaCodecList.h>
#include <media/MediaCodecInfo.h>

#include <gui/Surface.h>

#include <media/ICrypto.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/PersistentSurface.h>

#include <utils/Mutex.h>
#include <utils/Log.h>
#include <set>
#include <map>

namespace android {

/*
one MediaCodecList example
----------------------------------------------
MediaCodecInfo {
  name = OMX.rk.video_decoder.avc
  isEncoder = false
  Capabilities {
    mime = video/avc
    colorFormats[] = {OMX_COLOR_FormatYUV420Flexible, OMX_COLOR_FormatYUV420Planar, OMX_COLOR_FormatYUV420SemiPlanar}
  }
  // Usually one MediaCodecInfo has one Capabilities, but there may be more than one.
}

MediaCodecInfo {
  name = OMX.rk.video_decoder.vp8
  isEncoder = false
  Capabilities {
    mime = video/x-vnd.on2.vp8
    colorFormats[] = {OMX_COLOR_FormatYUV420Flexible, OMX_COLOR_FormatYUV420Planar, OMX_COLOR_FormatYUV420SemiPlanar}
  }
}

MediaCodecInfo {
  name = OMX.rk.video_decoder.hevc
  isEncoder = false
  Capabilities {
    mime = video/hevc
    colorFormats[] = {OMX_COLOR_FormatYUV420Flexible, OMX_COLOR_FormatYUV420Planar, OMX_COLOR_FormatYUV420SemiPlanar}
  }
}

MediaCodecInfo {
  name = OMX.rk.video_encoder.avc
  isEncoder = true
  Capabilities {
    mime = video/avc
    colorFormats[] = {OMX_COLOR_FormatYUV420Flexible, OMX_COLOR_FormatYUV420Planar, OMX_COLOR_FormatYUV420SemiPlanar, OMX_COLOR_FormatAndroidOpaque}
  }
}
*/

static sp<IMediaCodecList> getCodecList()
{
    sp<IMediaCodecList> mcl = MediaCodecList::getInstance();
    return mcl;
}

class MediaCodecInfo2
{
public:
	MediaCodecInfo2(const MediaCodecInfo& info);

	std::string codecName;
	bool isEncoder;

	struct tcap {
		std::string mime;
		std::set<int> colorFormats;
	};
	std::vector<tcap> caps;
};

MediaCodecInfo2::MediaCodecInfo2(const MediaCodecInfo& info)
{
	codecName = info.getCodecName();
	isEncoder = info.isEncoder();

	ALOGD("MediaCodecInfo2::MediaCodecInfo2(), E, codecName: %s, isEncoder: %s", codecName.c_str(), isEncoder? "true": "false");

	Vector<AString> mimes;
	info.getSupportedMimes(&mimes);

	for (size_t i = 0; i < mimes.size(); i ++) {
		const char *mimeStr = mimes.itemAt(i).c_str();
		
		const sp<MediaCodecInfo::Capabilities> cap = info.getCapabilitiesFor(mimeStr);

		caps.push_back(tcap());
		tcap& cap2 = caps.back();

		cap2.mime = mimeStr;
		ALOGD("MediaCodecInfo2::MediaCodecInfo2(), #%i: mime: %s, flags: 0x%x", (int)i, mimeStr, cap->getFlags());

		Vector<uint32_t> colorFormats;
		cap->getSupportedColorFormats(&colorFormats);
		for (size_t f = 0; f < colorFormats.size(); f ++) {
			cap2.colorFormats.insert(colorFormats.itemAt(f));
			ALOGD("  [%i/%i] format: 0x%x", (int)f, (int)colorFormats.size(), colorFormats.itemAt(f));
		}
	}
	ALOGD("MediaCodecInfo2::MediaCodecInfo2(), X");
}


class MediaCodecList2
{
public:
	static android::Mutex sInitLock;
	static std::vector<MediaCodecInfo2> sRegularCodecInfos;

	static void initCodecList();

	static void getCodecInfoAt(int index, kosMediaCodecInfo* info);
};

android::Mutex MediaCodecList2::sInitLock;
std::vector<MediaCodecInfo2> MediaCodecList2::sRegularCodecInfos;

void MediaCodecList2::initCodecList()
{
	android::Mutex::Autolock lock(sInitLock);
	if (sRegularCodecInfos.empty()) {
		sp<IMediaCodecList> mcl = getCodecList();
		int count = mcl->countCodecs();
		for (int index = 0; index < count; index ++) {
			const sp<MediaCodecInfo>& info = mcl->getCodecInfo(index);
			const MediaCodecInfo2 info2(*info.get());
			bool isRegular = !info2.caps.empty();
			if (isRegular) {
				sRegularCodecInfos.push_back(info2);
			}
			ALOGD("MediaCodecList2::initCodecList(), %i/%i------isRegular: %s", index, count, isRegular? "true": "false");
		}
		ALOGD("MediaCodecList2::initCodecList(), mcl->countCodecs(): %i, size: %i", (int)mcl->countCodecs(), (int)sRegularCodecInfos.size());
	}
}

void MediaCodecList2::getCodecInfoAt(int index, kosMediaCodecInfo* info)
{
	MediaCodecList2::initCodecList();
	memset(info, 0, sizeof(kosMediaCodecInfo));
	if (index < 0 || index >= (int)sRegularCodecInfos.size()) {
		return;
	}

	const MediaCodecInfo2& info2 = sRegularCodecInfos[index];
	strlcpy(info->name, info2.codecName.c_str(), sizeof(info->name));
	info->isEncoder = info2.isEncoder;

	int at = 0;
	for (std::vector<MediaCodecInfo2::tcap>::const_iterator it = info2.caps.begin(); it != info2.caps.end(); ++ it, at ++) {
		const MediaCodecInfo2::tcap& cap = *it;
		kosMediaCodecCap& cap2 = info->caps[at];
		strlcpy(cap2.mime, cap.mime.c_str(), sizeof(cap2.mime));
		int at2 = 0;
		for (std::set<int>::const_iterator it = cap.colorFormats.begin(); it != cap.colorFormats.end(); ++ it, at2 ++) {
			cap2.colorFormats[at2] = *it;
		}
	}
}

} //  namespace android

NDK_EXPORT int MediaCodecList_getCodecCount()
{
	android::MediaCodecList2::initCodecList();
	return android::MediaCodecList2::sRegularCodecInfos.size();
}

NDK_EXPORT void MediaCodecList_getCodecInfoAt(int index, kosMediaCodecInfo* info)
{	
	android::MediaCodecList2::getCodecInfoAt(index, info);
}

//
// android_media_MediaCodec.cpp
//
////////////////////////////////////////////////////////////////////////////////
namespace android {

static int status_t_2_dequeue_info(status_t err)
{
	switch (err) {
	case OK:
		return 0;

	case -EAGAIN:
		return DEQUEUE_INFO_TRY_AGAIN_LATER;

	case INFO_FORMAT_CHANGED:
		return DEQUEUE_INFO_OUTPUT_FORMAT_CHANGED;

	case INFO_OUTPUT_BUFFERS_CHANGED:
		return DEQUEUE_INFO_OUTPUT_BUFFERS_CHANGED;

	case INVALID_OPERATION:
		return DEQUEUE_INFO_OTHER_ERROR;

	case BAD_VALUE:
		return DEQUEUE_INFO_OTHER_ERROR;

	default:
		return DEQUEUE_INFO_OTHER_ERROR;
	}
}

class JMediaCodec: public AHandler 
{
public:
	static int uniqueId;
	static int nextUniqueId();

	JMediaCodec(const char *name, bool nameIsType, bool encoder);

    status_t initCheck() const;

    void registerSelf();
    void release();

    status_t configure(const kosMediaFormat* kosFormat, const sp<ICrypto> &crypto, int flags);

    status_t createInputSurface(sp<IGraphicBufferProducer>* bufferProducer);
    status_t setInputSurface(const sp<PersistentSurface> &surface);

    status_t start();
    status_t stop();
    status_t reset();

    status_t flush();

    bool queueInputBuffer(
            size_t index,
            size_t offset, size_t size, int64_t timeUs, uint32_t flags);

    status_t queueSecureInputBuffer(
            size_t index,
            size_t offset,
            const CryptoPlugin::SubSample *subSamples,
            size_t numSubSamples,
            const uint8_t key[16],
            const uint8_t iv[16],
            CryptoPlugin::Mode mode,
            const CryptoPlugin::Pattern &pattern,
            int64_t presentationTimeUs,
            uint32_t flags,
            AString *errorDetailMsg);

    int dequeueInputBuffer(int64_t timeoutUs);

    int dequeueOutputBuffer(kosBufferInfo* bufferInfo, int64_t timeoutUs);

    status_t releaseOutputBuffer(
            size_t index, bool render, bool updatePTS, int64_t timestampNs);

    status_t signalEndOfInputStream();

    status_t getFormat(bool input, kosMediaFormat *format) const;

    bool getOutputFormat(size_t index, kosMediaFormat *format) const;

    status_t getBuffers(bool input, Vector<sp<ABuffer> >& buffers) const;
	void freeAllTrackedBuffers();

	int getBufferLength(bool input) const;
    bool getBuffer(bool input, size_t index, kosABuffer* buffer) const;

    status_t getName(char* outName) const;

    status_t setParameters(const kosMediaCodecParameters& kosParams);

protected:
    virtual ~JMediaCodec();

    virtual void onMessageReceived(const sp<AMessage> &msg);

private:
	Mutex mBufferLock;
	Vector<sp<ABuffer> > mCachedInputBuffers;
    Vector<sp<ABuffer> > mCachedOutputBuffers;

    enum {
        kWhatCallbackNotify,
        kWhatFrameRendered,
    };

    sp<ALooper> mLooper;
    sp<MediaCodec> mCodec;

    status_t mInitStatus;

    DISALLOW_EVIL_CONSTRUCTORS(JMediaCodec);
};

static std::map<int, sp<JMediaCodec> > codecs;
static Mutex sCodecsLock;

int JMediaCodec::uniqueId = 0;

int JMediaCodec::nextUniqueId()
{
	do {
		if (uniqueId != INT_MAX) {
			uniqueId ++;
		} else {
			uniqueId = 0;
		}
	} while (codecs.count(uniqueId));

	return uniqueId;
}

JMediaCodec::JMediaCodec(const char *name, bool nameIsType, bool encoder)
{
	ALOGD("JMediaCodec::JMediaCodec(%s, nameIsType: %s, encoder: %s), E", name, nameIsType? "true": "false", encoder? "true": "false");
	mLooper = new ALooper;
	mLooper->setName("MediaCodec_looper");

	mLooper->start(
			false,      // runOnCallingThread
			true,       // canCallJava
			PRIORITY_FOREGROUND);

	if (nameIsType) {
		mCodec = MediaCodec::CreateByType(mLooper, name, encoder, &mInitStatus);
	} else {
		mCodec = MediaCodec::CreateByComponentName(mLooper, name, &mInitStatus);
	}
	CHECK((mCodec != NULL) != (mInitStatus != OK));

	ALOGD("JMediaCodec::JMediaCodec(%s, ...), X", name);
}

status_t JMediaCodec::initCheck() const 
{
	return mInitStatus;
}

void JMediaCodec::registerSelf() 
{
	mLooper->registerHandler(this);
}

void JMediaCodec::release()
{
	freeAllTrackedBuffers();
	if (mCodec != NULL) {
		mCodec->release();
		mCodec.clear();
		mInitStatus = NO_INIT;
	}

	if (mLooper != NULL) {
		mLooper->unregisterHandler(id());
		mLooper->stop();
		mLooper.clear();
	}
}

JMediaCodec::~JMediaCodec()
{
	ALOGD("JMediaCodec::~JMediaCodec(), E");

	if (mCodec != NULL || mLooper != NULL) {
		/* MediaCodec and looper should have been released explicitly already
			* in setMediaCodec() (see comments in setMediaCodec()).
			*
			* Otherwise JMediaCodec::~JMediaCodec() might be called from within the
			* message handler, doing release() there risks deadlock as MediaCodec::
			* release() post synchronous message to the same looper.
			*
			* Print a warning and try to proceed with releasing.
			*/
		ALOGW("try to release MediaCodec from JMediaCodec::~JMediaCodec()...");
		release();
		ALOGW("done releasing MediaCodec from JMediaCodec::~JMediaCodec().");
	}
	ALOGD("JMediaCodec::~JMediaCodec(), X");
}

#define KEY_MIME	"mime"
#define KEY_WIDTH	"width"
#define KEY_HEIGHT	"height"
#define KEY_COLOR_FORMAT	"color-format"
#define KEY_STRIDE	"stride"
#define KEY_SLICE_HEIGHT	"slice-height"

#define KEY_BIT_RATE    "bitrate"
#define KEY_BITRATE_MODE	"bitrate-mode"
#define KEY_FRAME_RATE	"frame-rate"
#define KEY_I_FRAME_INTERVAL	"i-frame-interval"
#define KEY_PROFILE	"profile"
#define KEY_LEVEL	"level"

static status_t ConvertMessageToMediaFormat(const sp<AMessage>& msg, kosMediaFormat* format)
{
	if (format == NULL) {
		return -EINVAL;
	}
	memset(format, 0, sizeof(kosMediaFormat));

	for (size_t i = 0; i < msg->countEntries(); ++i) {
		AMessage::Type valueType;
		const char *key = msg->getEntryNameAt(i, &valueType);

		if (!strncmp(key, "android._", 9)) {
			// don't expose private keys (starting with android._)
			continue;
		}

		switch (valueType) {
			case AMessage::kTypeInt32:
			{
				int32_t val;
				CHECK(msg->findInt32(key, &val));

				if (strcmp(key, KEY_WIDTH) == 0) {
					format->width = val;
				} else if (strcmp(key, KEY_HEIGHT) == 0) {
					format->height = val;
				} else if (strcmp(key, KEY_COLOR_FORMAT) == 0) {
					format->colorFormat = val;
				} else if (strcmp(key, KEY_STRIDE) == 0) {
					format->stride = val;
				} else if (strcmp(key, KEY_SLICE_HEIGHT) == 0) {
					format->sliceHeight = val;
				} else if (strcmp(key, KEY_BIT_RATE) == 0) {
					format->bitrate = val;
				} else if (strcmp(key, KEY_BITRATE_MODE) == 0) {
					format->bitrateMode = val;
				} else if (strcmp(key, KEY_FRAME_RATE) == 0) {
					format->frameRate = val;
				} else if (strcmp(key, KEY_I_FRAME_INTERVAL) == 0) {
					format->iFrameIntervalSec = val;
				} else if (strcmp(key, KEY_PROFILE) == 0) {
					format->profile = val;
				} else if (strcmp(key, KEY_LEVEL) == 0) {
					format->level = val;
				}

				break;
			}

			case AMessage::kTypeInt64:
			{
				int64_t val;
				CHECK(msg->findInt64(key, &val));

				break;
			}

			case AMessage::kTypeFloat:
			{
				float val;
				CHECK(msg->findFloat(key, &val));

				break;
			}

			case AMessage::kTypeString:
			{
				AString val;
				CHECK(msg->findString(key, &val));

				if (strcmp(key, KEY_MIME) == 0) {
					strlcpy(format->mime, val.c_str(), KOS_MAX_NAME_LEN);
				}
				break;
			}

			case AMessage::kTypeBuffer:
			{
				sp<ABuffer> buffer;
				CHECK(msg->findBuffer(key, &buffer));

				// valueObj = makeByteBufferObject(env, buffer->data(), buffer->size());
				break;
			}

			case AMessage::kTypeRect:
			{
				int32_t left, top, right, bottom;
				CHECK(msg->findRect(key, &left, &top, &right, &bottom));

				format->width = 1 + right - left;
				format->height = 1 + bottom - top;
				break;
			}

			default:
				break;
		}
	}

	return OK;
}

static status_t ConvertMediaFormatToMessage(const kosMediaFormat* format, sp<AMessage>* out)
{
	sp<AMessage> msg = new AMessage;
	if (format == NULL) {
		return -EINVAL;
	}

	if (format->mime[0] != '\0') {
		msg->setString(KEY_MIME, format->mime);
	}
	if (format->width != 0) {
		msg->setInt32(KEY_WIDTH, format->width);
	}
	if (format->height != 0) {
		msg->setInt32(KEY_HEIGHT, format->height);
	}
	if (format->colorFormat != 0) {
		msg->setInt32(KEY_COLOR_FORMAT, format->colorFormat);
	}
	if (format->stride != 0) {
		msg->setInt32(KEY_STRIDE, format->stride);
	}
	if (format->sliceHeight != 0) {
		msg->setInt32(KEY_SLICE_HEIGHT, format->sliceHeight);
	}

	if (format->bitrate != 0) {
		msg->setInt32(KEY_BIT_RATE, format->bitrate);
	}
	if (format->bitrateMode != 0) {
		msg->setInt32(KEY_BITRATE_MODE, format->bitrateMode);
	}
	if (format->frameRate != 0) {
		msg->setInt32(KEY_FRAME_RATE, format->frameRate);
	}
	if (format->iFrameIntervalSec != 0) {
		msg->setInt32(KEY_I_FRAME_INTERVAL, format->iFrameIntervalSec);
	}
	if (format->profile != 0) {
		msg->setInt32(KEY_PROFILE, format->profile);
	}
	if (format->level != 0) {
		msg->setInt32(KEY_LEVEL, format->level);
	}

	*out = msg;

	return OK;
}

status_t JMediaCodec::configure(const kosMediaFormat* kosFormat, const sp<ICrypto> &crypto, int flags)
{
	sp<AMessage> format;
	status_t err = ConvertMediaFormatToMessage(kosFormat, &format);
	if (err != OK) {
	    ALOGD("JMediaCodec::configure, E, fail, err: %i", err);
		return err;
	}

    kosMediaFormat kosFormat2;
    ConvertMessageToMediaFormat(format, &kosFormat2);
    ALOGD("mime: %s, width: %i, height: %i, stride: %i, sliceHeight: %i", kosFormat2.mime, kosFormat2.width, kosFormat2.height, kosFormat2.stride, kosFormat2.sliceHeight);

    sp<Surface> mSurfaceTextureClient;
	return mCodec->configure(format, mSurfaceTextureClient, crypto, flags);
}

status_t JMediaCodec::createInputSurface(sp<IGraphicBufferProducer>* bufferProducer)
{
	return mCodec->createInputSurface(bufferProducer);
}

status_t JMediaCodec::setInputSurface(const sp<PersistentSurface> &surface) 
{
	return mCodec->setInputSurface(surface);
}

status_t JMediaCodec::start()
{
    status_t err = mCodec->start();
	if (err != OK) {
		return err;
	}

	Mutex::Autolock lock(mBufferLock);
	getBuffers(true /* input */, mCachedInputBuffers);
	getBuffers(false /* input */, mCachedOutputBuffers);
	return OK;
}

status_t JMediaCodec::stop()
{
	status_t err = mCodec->stop();
	if (err != OK) {
		return err;
	}
	freeAllTrackedBuffers();
	return OK;
}

status_t JMediaCodec::flush()
{
    return mCodec->flush();
}

status_t JMediaCodec::reset() 
{
    return mCodec->reset();
}

bool JMediaCodec::queueInputBuffer(size_t index, size_t offset, size_t size, int64_t timeUs, uint32_t flags) 
{
	android::AString errorDetailMsg;
    status_t err = mCodec->queueInputBuffer(
            index, offset, size, timeUs, flags, &errorDetailMsg);
	return err == OK;
}

status_t JMediaCodec::queueSecureInputBuffer(
        size_t index,
        size_t offset,
        const CryptoPlugin::SubSample *subSamples,
        size_t numSubSamples,
        const uint8_t key[16],
        const uint8_t iv[16],
        CryptoPlugin::Mode mode,
        const CryptoPlugin::Pattern &pattern,
        int64_t presentationTimeUs,
        uint32_t flags,
        AString *errorDetailMsg) {
	return mCodec->queueSecureInputBuffer(
			index, offset, subSamples, numSubSamples, key, iv, mode, pattern,
			presentationTimeUs, flags, errorDetailMsg);
}

int JMediaCodec::dequeueInputBuffer(int64_t timeoutUs)
{
    size_t index;
	status_t err = mCodec->dequeueInputBuffer(&index, timeoutUs);
	if (err != OK) {
	    return status_t_2_dequeue_info(err);
	}
	return (int)index;
}

int JMediaCodec::dequeueOutputBuffer(kosBufferInfo* bufferInfo, int64_t timeoutUs)
{
	size_t index;
	size_t size, offset;
	int64_t timeUs;
	uint32_t flags;

	memset(bufferInfo, 0, sizeof(kosBufferInfo));
	status_t err = mCodec->dequeueOutputBuffer(&index, &offset, &size, &timeUs, &flags, timeoutUs);

	if (err != OK) {
		return status_t_2_dequeue_info(err);
	}

	bufferInfo->offset = offset;
	bufferInfo->size = size;
	bufferInfo->presentationTimeUs = timeUs;
	bufferInfo->flags = flags;

	return (int)index;
}

status_t JMediaCodec::releaseOutputBuffer(size_t index, bool render, bool updatePTS, int64_t timestampNs)
{
	if (updatePTS) {
		return mCodec->renderOutputBufferAndRelease(index, timestampNs);
	}
	return render? mCodec->renderOutputBufferAndRelease(index): mCodec->releaseOutputBuffer(index);
}

status_t JMediaCodec::signalEndOfInputStream()
{
	return mCodec->signalEndOfInputStream();
}

status_t JMediaCodec::getFormat(bool input, kosMediaFormat *format) const
{
	sp<AMessage> msg;
	status_t err;
	err = input ? mCodec->getInputFormat(&msg) : mCodec->getOutputFormat(&msg);
	if (err != OK) {
		return err;
	}

	return ConvertMessageToMediaFormat(msg, format);
}

bool JMediaCodec::getOutputFormat(size_t index, kosMediaFormat *format) const
{
	sp<AMessage> msg;
	status_t err;
	if ((err = mCodec->getOutputFormat(index, &msg)) != OK) {
		return false;
	}

	return ConvertMessageToMediaFormat(msg, format);
}

status_t JMediaCodec::getBuffers(bool input, Vector<sp<ABuffer> >& buffers) const
{
	buffers.clear();
    status_t err = input? mCodec->getInputBuffers(&buffers): mCodec->getOutputBuffers(&buffers);
    return err;
}

void JMediaCodec::freeAllTrackedBuffers()
{
	Mutex::Autolock lock(mBufferLock);

	mCachedInputBuffers.clear();
	mCachedOutputBuffers.clear();
}

int JMediaCodec::getBufferLength(bool input) const
{
	Vector<sp<ABuffer> > buffers = input? mCachedInputBuffers: mCachedOutputBuffers;
	return buffers.size();
}

bool JMediaCodec::getBuffer(bool input, size_t index, kosABuffer* kosbuffer) const
{
	Vector<sp<ABuffer> > buffers = input? mCachedInputBuffers: mCachedOutputBuffers;
	if (index >= buffers.size()) {
		return false;
	}
	const sp<ABuffer>& buffer = buffers.itemAt(index);

	kosbuffer->mData = buffer->base();
	kosbuffer->mCapacity = buffer->capacity();
	return true;
}

status_t JMediaCodec::getName(char* outName) const
{
    AString name;

    status_t err = mCodec->getName(&name);

    if (err != OK) {
        return err;
    }

	strlcpy(outName, name.c_str(), KOS_MAX_NAME_LEN);

    return OK;
}

#define PARAMETER_KEY_REQUEST_SYNC_FRAME	"request-sync"
#define PARAMETER_KEY_SUSPEND				"drop-input-frames"
#define PARAMETER_KEY_VIDEO_BITRATE			"video-bitrate"

static status_t ConvertParametersToMessage(const kosMediaCodecParameters& kosParams, sp<AMessage>* out)
{
	sp<AMessage> msg = new AMessage;

	if (kosParams.flags & KOS_PARAMETER_FLAG_REQUEST_SYNC_FRAME) {
		msg->setInt32(PARAMETER_KEY_REQUEST_SYNC_FRAME, kosParams.request_sync_frame);
	}
	if (kosParams.flags & KOS_PARAMETER_FLAG_VIDEO_BITRATE) {
		msg->setInt32(PARAMETER_KEY_VIDEO_BITRATE, kosParams.video_bitrate);
	}

	*out = msg;

	return OK;
}

status_t JMediaCodec::setParameters(const kosMediaCodecParameters& kosParams)
{
	if (kosParams.flags == 0) {
		return OK;
	}
    sp<AMessage> params;

	status_t err = ConvertParametersToMessage(kosParams, &params);
	if (err == OK) {
		err = mCodec->setParameters(params);
	}
	return err;
}

void JMediaCodec::onMessageReceived(const sp<AMessage> &msg)
{
    switch (msg->what()) {
        case kWhatCallbackNotify:
        case kWhatFrameRendered:
        default:
            TRESPASS();
    }
}

static sp<JMediaCodec> setMediaCodec(int id, const sp<JMediaCodec>& codec)
{
	Mutex::Autolock lock(sCodecsLock);
	std::map<int, sp<JMediaCodec> >::iterator find_it = codecs.find(id);
    sp<JMediaCodec> old = find_it != codecs.end()? find_it->second: NULL;
    if (old != NULL) {
        /* release MediaCodec and stop the looper now before decStrong.
         * otherwise JMediaCodec::~JMediaCodec() could be called from within
         * its message handler, doing release() from there will deadlock
         * (as MediaCodec::release() post synchronous message to the same looper)
         */
        old->release();
    }
	if (codec != NULL) {
		if (find_it == codecs.end()) {
			codecs.insert(std::make_pair(id, codec));
		}
	} else if (find_it != codecs.end()) {
		codecs.erase(find_it);
	}

    return old;
}

static sp<JMediaCodec> getMediaCodec(int id)
{
	Mutex::Autolock lock(sCodecsLock);
	return codecs.find(id)->second;
}

static int setupMediaCodec(const char* name, bool nameIsType, bool encoder)
{
	ALOGD("setupMediaCodec(%s, nameIsType: %s, encoder: %s), E", name, nameIsType? "true": "false", encoder? "true": "false");
    if (name == NULL) {
        return -EINVAL;
    }

    const char *tmp = name;

    sp<JMediaCodec> codec = new JMediaCodec(tmp, nameIsType, encoder);

    const status_t err = codec->initCheck();
    if (err == NAME_NOT_FOUND) {
        // fail and do not try again.
        // ("Failed to initialize %s, error %#x", tmp, err);
        ALOGD("setupMediaCodec(%s, ...), X, failed, NAME_NOT_FOUND", name);
        return -1;
    } if (err == NO_MEMORY) {
        // ("Failed to initialize %s, error %#x", tmp, err);
        ALOGD("setupMediaCodec(%s, ...), X, failed, NO_MEMORY", name);
        return -1;
    } else if (err != OK) {
        // believed possible to try again
        // ("Failed to find matching codec %s, error %#x", tmp, err)
        ALOGD("setupMediaCodec(%s, ...), X, error: %i", name, err);
        return -1;
    }

    codec->registerSelf();

	int handle = JMediaCodec::nextUniqueId();
    setMediaCodec(handle, codec);

	ALOGD("setupMediaCodec(%s, ...), X, handle: %i, codecs.size: %i", name, handle, (int)codecs.size());
	return handle;
}

}

NDK_EXPORT kosMediaFormat MediaFormat_create(const char* mime, int width, int height)
{
	kosMediaFormat format;
	memset(&format, 0, sizeof(kosMediaFormat));

	strlcpy(format.mime, mime, sizeof(format.mime));
	format.width = width;
	format.height = height;
	return format;
}

NDK_EXPORT int MediaCodec_setup(const char* name)
{
	return android::setupMediaCodec(name, false /* nameIsType */, false /* unused */);
}

NDK_EXPORT int MediaCodec_configure(int handle, kosMediaFormat* format, int flags)
{
	android::sp<android::JMediaCodec> codec = android::getMediaCodec(handle);
	if (codec == NULL) {
		return -EINVAL;
	}
	return codec->configure(format, NULL, flags);
}

NDK_EXPORT void MediaCodec_start(int handle)
{
	android::sp<android::JMediaCodec> codec = android::getMediaCodec(handle);
	if (codec == NULL) {
		return;
	}
	codec->start();
}

NDK_EXPORT void MediaCodec_stop(int handle)
{
	android::sp<android::JMediaCodec> codec = android::getMediaCodec(handle);
	if (codec == NULL) {
		return;
	}
	codec->stop();
}

NDK_EXPORT void MediaCodec_flush(int handle)
{
	android::sp<android::JMediaCodec> codec = android::getMediaCodec(handle);
	if (codec == NULL) {
		return;
	}
	codec->flush();
}

NDK_EXPORT void MediaCodec_release(int handle)
{
	android::sp<android::JMediaCodec> codec = android::getMediaCodec(handle);
	if (codec == NULL) {
		return;
	}
	android::setMediaCodec(handle, NULL);
}

NDK_EXPORT int MediaCodec_getBufferLength(int handle, bool input)
{
	android::sp<android::JMediaCodec> codec = android::getMediaCodec(handle);
	if (codec == NULL) {
		return 0;
	}
	return codec->getBufferLength(input);
}

NDK_EXPORT bool MediaCodec_getBuffer(int handle, bool input, int index, kosABuffer* buffer)
{
	android::sp<android::JMediaCodec> codec = android::getMediaCodec(handle);
	if (codec == NULL) {
		return false;
	}
	return codec->getBuffer(input, index, buffer);
}

NDK_EXPORT int MediaCodec_dequeueInputBuffer(int handle, int64_t timeoutUs)
{
	android::sp<android::JMediaCodec> codec = android::getMediaCodec(handle);
	if (codec == NULL) {
		return DEQUEUE_INFO_OTHER_ERROR;
	}
	return codec->dequeueInputBuffer(timeoutUs);
}

NDK_EXPORT bool MediaCodec_queueInputBuffer(int handle, int index, size_t offset, size_t size, int64_t presentationTimeStamUs)
{
	android::sp<android::JMediaCodec> codec = android::getMediaCodec(handle);
	if (codec == NULL) {
		return false;
	}
	return codec->queueInputBuffer(index, offset, size, presentationTimeStamUs, 0);
}

NDK_EXPORT int MediaCodec_dequeueOutputBuffer(int handle, int64_t timeoutUs, kosBufferInfo* bufferInfo)
{
	android::sp<android::JMediaCodec> codec = android::getMediaCodec(handle);
	if (codec == NULL) {
		return DEQUEUE_INFO_OTHER_ERROR;
	}
	return codec->dequeueOutputBuffer(bufferInfo, timeoutUs);
}

NDK_EXPORT void MediaCodec_releaseOutputBuffer(int handle, int index)
{
	android::sp<android::JMediaCodec> codec = android::getMediaCodec(handle);
	if (codec == NULL) {
		return;
	}
	codec->releaseOutputBuffer(index, false /*render*/, false /* updatePTS */, 0 /* dummy */);
}

NDK_EXPORT bool MediaCodec_getOutputFormat(int handle, kosMediaFormat* format)
{
	android::sp<android::JMediaCodec> codec = android::getMediaCodec(handle);
	if (codec == NULL) {
		return false;
	}
	return codec->getFormat(false, format);
}

NDK_EXPORT kosMediaCodecParameters MediaCodec_createParameters(uint32_t flags)
{
	kosMediaCodecParameters params;
	memset(&params, 0, sizeof(kosMediaCodecParameters));

	params.flags = flags;
	return params;
}

NDK_EXPORT void MediaCodec_setParameters(int handle, kosMediaCodecParameters* params)
{
    android::sp<android::JMediaCodec> codec = android::getMediaCodec(handle);
    if (codec == NULL) {
    	return;
    }
    codec->setParameters(*params);
}