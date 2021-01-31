/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <termios.h>
#include <unistd.h>

#define LOG_TAG "ScreenRecord"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS
// #define LOG_NDEBUG 0
#include <utils/Log.h>

#include <binder/IPCThreadState.h>
#include <utils/Errors.h>
#include <utils/Timers.h>
#include <utils/Trace.h>

#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>
#include <ui/DisplayInfo.h>
#include <media/openmax/OMX_IVCommon.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaMuxer.h>
#include <media/ICrypto.h>

#include "screenrecord.h"
// #include "Overlay.h"
#include "FrameOutput.h"
// #include "eventhub.h"
#include "sendinput.h"
#include <kosapi/sys.h>
// #include <kosapi/net.h>

using namespace android;

static const uint32_t kMinBitRate = 100000;         // 0.1Mbps
static const uint32_t kMaxBitRate = 200 * 1000000;  // 200Mbps
static const uint32_t kMaxTimeLimitSec = 180;       // 3 minutes
static const uint32_t kFallbackWidth = 1280;        // 720p
static const uint32_t kFallbackHeight = 720;
static const char* kMimeTypeAvc = "video/avc";

// Command-line parameters.
static bool gVerbose = false;           // chatty on stdout
static bool gRotate = false;            // rotate 90 degrees
static enum {
    FORMAT_MP4, FORMAT_H264, FORMAT_FRAMES, FORMAT_RAW_FRAMES
} gOutputFormat = FORMAT_MP4;           // data format for output
static bool gSizeSpecified = false;     // was size explicitly requested?
static uint32_t gVideoWidth = 0;        // default width+height
static uint32_t gVideoHeight = 0;
static uint32_t gBitRate = 4000000;     // 4Mbps
static uint32_t gTimeLimitSec = kMaxTimeLimitSec;
static bool gPause = false;
static bool gRequireSetPause = false;
// EventHub* gEventHubPtr = nullptr;
// NativeConnection* gConnectionPtr = nullptr;

// Set by signal handler to stop recording.
static volatile bool gStopRequested = false;

// Previous signal handler state, restored after first hit.
static struct sigaction gOrigSigactionINT;
static struct sigaction gOrigSigactionHUP;


/*
 * Catch keyboard interrupt signals.  On receipt, the "stop requested"
 * flag is raised, and the original handler is restored (so that, if
 * we get stuck finishing, a second Ctrl-C will kill the process).
 */
static void signalCatcher(int signum)
{
    gStopRequested = true;
    switch (signum) {
    case SIGINT:
    case SIGHUP:
        sigaction(SIGINT, &gOrigSigactionINT, NULL);
        sigaction(SIGHUP, &gOrigSigactionHUP, NULL);
        break;
    default:
        abort();
        break;
    }
}

/*
 * Configures signal handlers.  The previous handlers are saved.
 *
 * If the command is run from an interactive adb shell, we get SIGINT
 * when Ctrl-C is hit.  If we're run from the host, the local adb process
 * gets the signal, and we get a SIGHUP when the terminal disconnects.
 */
static status_t configureSignals() {
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = signalCatcher;
    if (sigaction(SIGINT, &act, &gOrigSigactionINT) != 0) {
        status_t err = -errno;
        fprintf(stderr, "Unable to configure SIGINT handler: %s\n",
                strerror(errno));
        return err;
    }
    if (sigaction(SIGHUP, &act, &gOrigSigactionHUP) != 0) {
        status_t err = -errno;
        fprintf(stderr, "Unable to configure SIGHUP handler: %s\n",
                strerror(errno));
        return err;
    }
    return NO_ERROR;
}

/*
 * Returns "true" if the device is rotated 90 degrees.
 */
static bool isDeviceRotated(int orientation) {
    return orientation != DISPLAY_ORIENTATION_0 &&
            orientation != DISPLAY_ORIENTATION_180;
}

/*
 * Configures and starts the MediaCodec encoder.  Obtains an input surface
 * from the codec.
 */
static status_t prepareEncoder(float displayFps, sp<MediaCodec>* pCodec,
        sp<IGraphicBufferProducer>* pBufferProducer) {
    status_t err;

    ALOGD("Configuring recorder for %dx%d %s at %.2fMbps, fps: %.2f",
                    gVideoWidth, gVideoHeight, kMimeTypeAvc, gBitRate / 1000000.0, displayFps);

    sp<AMessage> format = new AMessage;
    format->setInt32("width", gVideoWidth);
    format->setInt32("height", gVideoHeight);
    format->setString("mime", kMimeTypeAvc);
    format->setInt32("color-format", OMX_COLOR_FormatAndroidOpaque);
    format->setInt32("bitrate", gBitRate);
    format->setFloat("frame-rate", displayFps); // must set, else not encoded frame output.
    format->setInt32("i-frame-interval", 10);
    // must restrict fps to encoder.
    format->setFloat("max-fps-to-encoder", displayFps);

    sp<android::ALooper> looper = new android::ALooper;
    looper->setName("screenrecord_looper");
    looper->start();
    ALOGV("Creating codec");
    sp<MediaCodec> codec = MediaCodec::CreateByType(looper, kMimeTypeAvc, true);
    if (codec == NULL) {
        fprintf(stderr, "ERROR: unable to create %s codec instance\n",
                kMimeTypeAvc);
        return UNKNOWN_ERROR;
    }

    err = codec->configure(format, NULL, NULL,
            MediaCodec::CONFIGURE_FLAG_ENCODE);
    if (err != NO_ERROR) {
        fprintf(stderr, "ERROR: unable to configure %s codec at %dx%d (err=%d)\n",
                kMimeTypeAvc, gVideoWidth, gVideoHeight, err);
        codec->release();
        return err;
    }

    ALOGV("Creating encoder input surface");
    sp<IGraphicBufferProducer> bufferProducer;
    err = codec->createInputSurface(&bufferProducer);
    if (err != NO_ERROR) {
        fprintf(stderr,
            "ERROR: unable to create encoder input surface (err=%d)\n", err);
        codec->release();
        return err;
    }

    ALOGV("Starting codec");
    err = codec->start();
    if (err != NO_ERROR) {
        fprintf(stderr, "ERROR: unable to start codec (err=%d)\n", err);
        codec->release();
        return err;
    }

    ALOGV("Codec prepared");
    *pCodec = codec;
    *pBufferProducer = bufferProducer;
    return 0;
}

/*
 * Sets the display projection, based on the display dimensions, video size,
 * and device orientation.
 */
static status_t setDisplayProjection(const sp<IBinder>& dpy,
        const DisplayInfo& mainDpyInfo) {
    status_t err;

    // Set the region of the layer stack we're interested in, which in our
    // case is "all of it".  If the app is rotated (so that the width of the
    // app is based on the height of the display), reverse width/height.
    bool deviceRotated = isDeviceRotated(mainDpyInfo.orientation);
    uint32_t sourceWidth, sourceHeight;
    if (!deviceRotated) {
        sourceWidth = mainDpyInfo.w;
        sourceHeight = mainDpyInfo.h;
    } else {
        ALOGV("using rotated width/height");
        sourceHeight = mainDpyInfo.w;
        sourceWidth = mainDpyInfo.h;
    }
    Rect layerStackRect(sourceWidth, sourceHeight);

    // We need to preserve the aspect ratio of the display.
    float displayAspect = (float) sourceHeight / (float) sourceWidth;


    // Set the way we map the output onto the display surface (which will
    // be e.g. 1280x720 for a 720p video).  The rect is interpreted
    // post-rotation, so if the display is rotated 90 degrees we need to
    // "pre-rotate" it by flipping width/height, so that the orientation
    // adjustment changes it back.
    //
    // We might want to encode a portrait display as landscape to use more
    // of the screen real estate.  (If players respect a 90-degree rotation
    // hint, we can essentially get a 720x1280 video instead of 1280x720.)
    // In that case, we swap the configured video width/height and then
    // supply a rotation value to the display projection.
    uint32_t videoWidth, videoHeight;
    uint32_t outWidth, outHeight;
    if (!gRotate) {
        videoWidth = gVideoWidth;
        videoHeight = gVideoHeight;
    } else {
        videoWidth = gVideoHeight;
        videoHeight = gVideoWidth;
    }
    if (videoHeight > (uint32_t)(videoWidth * displayAspect)) {
        // limited by narrow width; reduce height
        outWidth = videoWidth;
        outHeight = (uint32_t)(videoWidth * displayAspect);
    } else {
        // limited by short height; restrict width
        outHeight = videoHeight;
        outWidth = (uint32_t)(videoHeight / displayAspect);
    }
    uint32_t offX, offY;
    offX = (videoWidth - outWidth) / 2;
    offY = (videoHeight - outHeight) / 2;
    Rect displayRect(offX, offY, offX + outWidth, offY + outHeight);

    {
        if (gRotate) {
            ALOGD("setDisplayProjection, Rotated content area is %ux%u at offset x=%d y=%d displayAspect=%.2f",
                    outHeight, outWidth, offY, offX, displayAspect);
        } else {
            ALOGD("setDisplayProjection, Content area is %ux%u at offset x=%d y=%d displayAspect=%.2f",
                    outWidth, outHeight, offX, offY, displayAspect);
        }
    }

    SurfaceComposerClient::setDisplayProjection(dpy,
            gRotate ? DISPLAY_ORIENTATION_90 : DISPLAY_ORIENTATION_0,
            layerStackRect, displayRect);
    return NO_ERROR;
}

/*
 * Configures the virtual display.  When this completes, virtual display
 * frames will start arriving from the buffer producer.
 */
static status_t prepareVirtualDisplay(const DisplayInfo& mainDpyInfo,
        const sp<IGraphicBufferProducer>& bufferProducer,
        sp<IBinder>* pDisplayHandle) {
    sp<IBinder> dpy = SurfaceComposerClient::createDisplay(
            String8("ScreenRecorder"), false /*secure*/);

    SurfaceComposerClient::openGlobalTransaction();
    SurfaceComposerClient::setDisplaySurface(dpy, bufferProducer);
    setDisplayProjection(dpy, mainDpyInfo);
    SurfaceComposerClient::setDisplayLayerStack(dpy, 0);    // default stack
    SurfaceComposerClient::closeGlobalTransaction();

    *pDisplayHandle = dpy;

    return NO_ERROR;
}

/*
 * Runs the MediaCodec encoder, sending the output to the MediaMuxer.  The
 * input frames are coming from the virtual display as fast as SurfaceFlinger
 * wants to send them.
 *
 * Exactly one of muxer or rawFp must be non-null.
 *
 * The muxer must *not* have been started before calling.
 */
static status_t runEncoder(const sp<MediaCodec>& encoder,
        const sp<MediaMuxer>& muxer, uint8_t* pixelBuf, fdid_gui2_screen_captured didScreenCaptured, void* user, const sp<IBinder>& mainDpy,
        const sp<IBinder>& virtualDpy, uint8_t orientation) {
    static int kTimeout = 250000;   // be responsive on signal
    status_t err;
    ssize_t trackIdx = -1;
    uint32_t debugNumFrames = 0;
    int64_t startWhenNsec = systemTime(CLOCK_MONOTONIC);
    int64_t endWhenNsec = startWhenNsec + seconds_to_nanoseconds(gTimeLimitSec);
    DisplayInfo mainDpyInfo;
    uint32_t callDequeueOutputBufferTimes = 0;
    std::unique_ptr<uint8_t> configData;
    int configDataSize = 0;
    // const char* name, const char* uniqueId, int32_t width, int32_t height, int32_t maxPointers
    // gConnectionPtr = NativeConnection::open("RDP uinput", "com.kos.launcher", gVideoWidth, gVideoHeight, 1);
    // EventHub eventHub;
    // eventHub.start();
    // gEventHubPtr = &eventHub;

    // assert((rawFp == NULL && muxer != NULL) || (rawFp != NULL && muxer == NULL));

    Vector<sp<ABuffer> > buffers;
    err = encoder->getOutputBuffers(&buffers);
    if (err != NO_ERROR) {
        fprintf(stderr, "Unable to get output buffers (err=%d)\n", err);
        return err;
    }

    // Run until we're signaled.
    while (!gStopRequested) {
        size_t bufIndex, offset, size;
        int64_t ptsUsec;
        uint32_t flags;
        uint32_t flags2 = 0;
/*
        if (systemTime(CLOCK_MONOTONIC) > endWhenNsec) {
            if (gVerbose) {
                printf("Time limit reached\n");
            }
            break;
        }
*/
        // eventHub.loopOnce();
        if (gRequireSetPause) {
            gRequireSetPause = false;
            sp<AMessage> params = new AMessage;
            params->setInt32("drop-input-frames", gPause);
            encoder->setParameters(params);
        }

        ALOGV("#%u Calling dequeueOutputBuffer", callDequeueOutputBufferTimes);
        err = encoder->dequeueOutputBuffer(&bufIndex, &offset, &size, &ptsUsec,
                &flags, kTimeout);
        ALOGV("#%u dequeueOutputBuffer returned %d", callDequeueOutputBufferTimes, err);
        callDequeueOutputBufferTimes ++;
        switch (err) {
        case NO_ERROR:
            // got a buffer
            if ((flags & MediaCodec::BUFFER_FLAG_CODECCONFIG) != 0) {
                ALOGV("Got codec config buffer (%zu bytes)", size);
                if (muxer != NULL) {
                    // ignore this -- we passed the CSD into MediaMuxer when
                    // we got the format change notification
                    size = 0;
                } else {
                    ALOGV("Save %zu bytes to configData, then zero size", size);
                    configData.reset(new uint8_t[size]);
                    memcpy(configData.get(), buffers[bufIndex]->data(), size);
                    configDataSize = size;
                    size = 0;
                }
            }
            if (size != 0) {
                ALOGV("Got data in buffer %zu, size=%zu, pts=%" PRId64,
                        bufIndex, size, ptsUsec);

                { // scope
                    ATRACE_NAME("orientation");
                    // Check orientation, update if it has changed.
                    //
                    // Polling for changes is inefficient and wrong, but the
                    // useful stuff is hard to get at without a Dalvik VM.
                    err = SurfaceComposerClient::getDisplayInfo(mainDpy,
                            &mainDpyInfo);
                    if (err != NO_ERROR) {
                        ALOGW("getDisplayInfo(main) failed: %d", err);
                    } else if (orientation != mainDpyInfo.orientation) {
                        ALOGD("orientation changed, now %d", mainDpyInfo.orientation);
                        SurfaceComposerClient::openGlobalTransaction();
                        setDisplayProjection(virtualDpy, mainDpyInfo);
                        SurfaceComposerClient::closeGlobalTransaction();
                        orientation = mainDpyInfo.orientation;
                    }
                }

                // If the virtual display isn't providing us with timestamps,
                // use the current time.  This isn't great -- we could get
                // decoded data in clusters -- but we're not expecting
                // to hit this anyway.
                if (ptsUsec == 0) {
                    ptsUsec = systemTime(SYSTEM_TIME_MONOTONIC) / 1000;
                }

                if (muxer == NULL) {
                    // fwrite(buffers[bufIndex]->data(), 1, size, rawFp);
                    if ((flags & MediaCodec::BUFFER_FLAG_SYNCFRAME) != 0) {
                        ALOGD("It is SYNCFRAME, copy configData(%i bytes), gVideoWidth: %i, gVideoHeight: %i", configDataSize, gVideoWidth, gVideoHeight);
                        flags2 |= KOS_RECORDSCREEN_FLAG_SYNCFRAME;
                        memcpy(pixelBuf, configData.get(), configDataSize);
                        memcpy(pixelBuf + configDataSize, buffers[bufIndex]->data(), size);
                        size += configDataSize;
                    } else {
                        memcpy(pixelBuf, buffers[bufIndex]->data(), size);
                    }
/*
                    {
                        static uint32_t time = 0;
                        clock_t t = clock();
                        ALOGD("[#%u]%u(%i) didScreenCaptured(%p, %u, %i, %i)", time ++, (uint32_t)(t / (CLOCKS_PER_SEC / 1000)), CLOCKS_PER_SEC, pixelBuf, (uint32_t)size, (int)gVideoWidth, (int)gVideoHeight);
                    }
*/
                    flags2 |= mainDpyInfo.orientation;
                    didScreenCaptured(pixelBuf, size, gVideoWidth, gVideoHeight, flags2, user);
                    // ALOGV("post didScreenCaptured");
/*
                    // Flush the data immediately in case we're streaming.
                    // We don't want to do this if all we've written is
                    // the SPS/PPS data because mplayer gets confused.
                    if ((flags & MediaCodec::BUFFER_FLAG_CODECCONFIG) == 0) {
                        ALOGV("this frame doesn't set flag: MediaCodec::BUFFER_FLAG_CODECCONFIG");
                        // fflush(rawFp);
                    }
*/
                } else {
                    // The MediaMuxer docs are unclear, but it appears that we
                    // need to pass either the full set of BufferInfo flags, or
                    // (flags & BUFFER_FLAG_SYNCFRAME).
                    //
                    // If this blocks for too long we could drop frames.  We may
                    // want to queue these up and do them on a different thread.
/*
                    ATRACE_NAME("write sample");
                    assert(trackIdx != -1);
                    err = muxer->writeSampleData(buffers[bufIndex], trackIdx,
                            ptsUsec, flags);
                    if (err != NO_ERROR) {
                        fprintf(stderr,
                            "Failed writing data to muxer (err=%d)\n", err);
                        return err;
                    }
*/
                }
                debugNumFrames++;
            }
            err = encoder->releaseOutputBuffer(bufIndex);
            if (err != NO_ERROR) {
                fprintf(stderr, "Unable to release output buffer (err=%d)\n",
                        err);
                return err;
            }
            if ((flags & MediaCodec::BUFFER_FLAG_EOS) != 0) {
                // Not expecting EOS from SurfaceFlinger.  Go with it.
                ALOGI("Received end-of-stream");
                gStopRequested = true;
            }
            break;
        case -EAGAIN:                       // INFO_TRY_AGAIN_LATER
            ALOGV("Got -EAGAIN, looping");
            break;
        case INFO_FORMAT_CHANGED:           // INFO_OUTPUT_FORMAT_CHANGED
            {
                // Format includes CSD, which we must provide to muxer.
                ALOGV("Encoder format changed");
                sp<AMessage> newFormat;
                encoder->getOutputFormat(&newFormat);
                if (muxer != NULL) {
                    trackIdx = muxer->addTrack(newFormat);
                    ALOGV("Starting muxer");
                    err = muxer->start();
                    if (err != NO_ERROR) {
                        fprintf(stderr, "Unable to start muxer (err=%d)\n", err);
                        return err;
                    }
                }
            }
            break;
        case INFO_OUTPUT_BUFFERS_CHANGED:   // INFO_OUTPUT_BUFFERS_CHANGED
            // Not expected for an encoder; handle it anyway.
            ALOGV("Encoder buffers changed");
            err = encoder->getOutputBuffers(&buffers);
            if (err != NO_ERROR) {
                fprintf(stderr,
                        "Unable to get new output buffers (err=%d)\n", err);
                return err;
            }
            break;
        case INVALID_OPERATION:
            ALOGW("dequeueOutputBuffer returned INVALID_OPERATION");
            return err;
        default:
            fprintf(stderr,
                    "Got weird result %d from dequeueOutputBuffer\n", err);
            return err;
        }
    }

    ALOGV("Encoder stopping (req=%d)", gStopRequested);
    if (gVerbose) {
        printf("Encoder stopping; recorded %u frames in %" PRId64 " seconds\n",
                debugNumFrames, nanoseconds_to_seconds(
                        systemTime(CLOCK_MONOTONIC) - startWhenNsec));
    }
    return NO_ERROR;
}

void prepareInputDevice()
{

}

/*
 * Main "do work" start point.
 *
 * Configures codec, muxer, and virtual display, then starts moving bits
 * around.
 */
static status_t recordScreen(uint32_t max_fps_to_encoder, uint8_t* pixelBuf, fdid_gui2_screen_captured didScreenCaptured, void* user) {
    status_t err;
/*
    // Configure signal handler.
    err = configureSignals();
    if (err != NO_ERROR) return err;

    // Start Binder thread pool.  MediaCodec needs to be able to receive
    // messages from mediaserver.
    sp<ProcessState> self = ProcessState::self();
    self->startThreadPool();
*/
    // Get main display parameters.
    sp<IBinder> mainDpy = SurfaceComposerClient::getBuiltInDisplay(
            ISurfaceComposer::eDisplayIdMain);
    DisplayInfo mainDpyInfo;
    err = SurfaceComposerClient::getDisplayInfo(mainDpy, &mainDpyInfo);
    if (err != NO_ERROR) {
        ALOGE("ERROR: unable to get display characteristics");
        return err;
    }

    ALOGD("Main display is %dx%d @%.2ffps (orientation=%u)",
                mainDpyInfo.w, mainDpyInfo.h, mainDpyInfo.fps,
                mainDpyInfo.orientation);
    float max_fps_to_encoder2 = max_fps_to_encoder == 0 || max_fps_to_encoder > mainDpyInfo.fps? mainDpyInfo.fps: max_fps_to_encoder;


    bool rotated = isDeviceRotated(mainDpyInfo.orientation);
    if (gVideoWidth == 0) {
        gVideoWidth = rotated ? mainDpyInfo.h : mainDpyInfo.w;
    }
    if (gVideoHeight == 0) {
        gVideoHeight = rotated ? mainDpyInfo.w : mainDpyInfo.h;
    }

    // Configure and start the encoder.
    sp<MediaCodec> encoder;
    sp<FrameOutput> frameOutput;
    sp<IGraphicBufferProducer> encoderInputSurface;
    if (gOutputFormat != FORMAT_FRAMES && gOutputFormat != FORMAT_RAW_FRAMES) {
        err = prepareEncoder(max_fps_to_encoder2, &encoder, &encoderInputSurface);

        if (err != NO_ERROR && !gSizeSpecified) {
            // fallback is defined for landscape; swap if we're in portrait
            bool needSwap = gVideoWidth < gVideoHeight;
            uint32_t newWidth = needSwap ? kFallbackHeight : kFallbackWidth;
            uint32_t newHeight = needSwap ? kFallbackWidth : kFallbackHeight;
            if (gVideoWidth != newWidth && gVideoHeight != newHeight) {
                ALOGV("Retrying with 720p");
                fprintf(stderr, "WARNING: failed at %dx%d, retrying at %dx%d\n",
                        gVideoWidth, gVideoHeight, newWidth, newHeight);
                gVideoWidth = newWidth;
                gVideoHeight = newHeight;
                err = prepareEncoder(max_fps_to_encoder2, &encoder,
                        &encoderInputSurface);
            }
        }
        if (err != NO_ERROR) return err;

        // From here on, we must explicitly release() the encoder before it goes
        // out of scope, or we will get an assertion failure from stagefright
        // later on in a different thread.
    } else {
        // We're not using an encoder at all.  The "encoder input surface" we hand to
        // SurfaceFlinger will just feed directly to us.
        frameOutput = new FrameOutput();
        err = frameOutput->createInputSurface(gVideoWidth, gVideoHeight, &encoderInputSurface);
        if (err != NO_ERROR) {
            return err;
        }
    }


    sp<IGraphicBufferProducer> bufferProducer;
    // Use the encoder's input surface as the virtual display surface.
    bufferProducer = encoderInputSurface;


    // Configure virtual display.
    sp<IBinder> dpy;
    err = prepareVirtualDisplay(mainDpyInfo, bufferProducer, &dpy);
    if (err != NO_ERROR) {
        if (encoder != NULL) encoder->release();
        return err;
    }

    sp<MediaMuxer> muxer = NULL;
    if (gOutputFormat == FORMAT_FRAMES || gOutputFormat == FORMAT_RAW_FRAMES) {
        // TODO: if we want to make this a proper feature, we should output
        //       an outer header with version info.  Right now we never change
        //       the frame size or format, so we could conceivably just send
        //       the current frame header once and then follow it with an
        //       unbroken stream of data.

        // Make the EGL context current again.  This gets unhooked if we're
        // using "--bugreport" mode.
        // TODO: figure out if we can eliminate this
        frameOutput->prepareToCopy();

        while (!gStopRequested) {
            // Poll for frames, the same way we do for MediaCodec.  We do
            // all of the work on the main thread.
            //
            // Ideally we'd sleep indefinitely and wake when the
            // stop was requested, but this will do for now.  (It almost
            // works because wait() wakes when a signal hits, but we
            // need to handle the edge cases.)
            bool rawFrames = gOutputFormat == FORMAT_RAW_FRAMES;
            err = frameOutput->copyFrame(pixelBuf, didScreenCaptured, user, 250000);
            if (err == ETIMEDOUT) {
                err = NO_ERROR;
            } else if (err != NO_ERROR) {
                ALOGE("Got error %d from copyFrame()", err);
                break;
            }
        }
    } else {
        // Main encoder loop.
        err = runEncoder(encoder, muxer, pixelBuf, didScreenCaptured, user, mainDpy, dpy,
                mainDpyInfo.orientation);
        // gEventHubPtr = nullptr;
        // delete gConnectionPtr;
        // gConnectionPtr = nullptr;
        if (err != NO_ERROR) {
            fprintf(stderr, "Encoder failed (err=%d)\n", err);
            // fall through to cleanup
        }

        if (gVerbose) {
            printf("Stopping encoder and muxer\n");
        }
    }

    // Shut everything down, starting with the producer side.
    encoderInputSurface = NULL;
    SurfaceComposerClient::destroyDisplay(dpy);
    if (encoder != NULL) {
        ALOGD("call encoder->stop()");
        encoder->stop();
    }
    if (muxer != NULL) {
        // If we don't stop muxer explicitly, i.e. let the destructor run,
        // it may hang (b/11050628).
        err = muxer->stop();
    } /* else if (rawFp != stdout) {
        fclose(rawFp);
    } */
    if (encoder != NULL) {
        ALOGD("call encoder->release()");
        encoder->release();
        encoder.clear();
    }

    return err;
}

NDK_EXPORT int kosRecordScreenLoop(uint32_t bitrate_kbps, uint32_t max_fps_to_encoder, uint8_t* pixel_buf, fdid_gui2_screen_captured did, void* user) {
    ALOGD("screenrecord.cpp::kosRecordScreenLoop---max_fps_to_encoder: %u", max_fps_to_encoder);
    // char msg[64];
    // kosNetGetCfg(msg, sizeof(msg));
    // kosNetSetCfg("interface setcfg eth0 192.168.1.116 24 multicast up broadcast running");
    gVerbose = true;
    gVideoWidth = 0; // the orientation when this record started maybe different from last.
    gVideoHeight = 0;
    gOutputFormat = bitrate_kbps > 0? FORMAT_H264: FORMAT_RAW_FRAMES;
    gRotate = false;
    gStopRequested = false;
    if (bitrate_kbps > 0) {
        gBitRate = bitrate_kbps * 1000;
    }

    ALOGD("gOutputFormat: %i, gBitRate: %u", gOutputFormat, gBitRate);

    status_t err = recordScreen(max_fps_to_encoder, pixel_buf, did, user);
    gPause = false;
    gRequireSetPause = false;
    ALOGD("---screenrecord.cpp::kosRecordScreenLoop X err: %s", err == NO_ERROR ? "success" : "failed");
    return (int) err;
}

NDK_EXPORT void kosStopRecordScreen()
{
    gStopRequested = true;
}

NDK_EXPORT void kosPauseRecordScreen(bool pause)
{
    if (gPause == pause) {
        return;
    }
    // to VirtualDisplay, SurfaceComposerClient::setDisplayPowerMode(dpy, mode) is no effect. see SurfaceFlinger::setPowerMode.
    // setVirtualDisplayMode(gDpy, pause? HWC_POWER_MODE_OFF: HWC_POWER_MODE_NORMAL);
    gPause = pause;
    gRequireSetPause = true;
}

NDK_EXPORT bool kosRecordScreenPaused()
{
    return gPause;
}

NDK_EXPORT void kosGetDisplayInfo(KosDisplayInfo* info)
{
    memset(info, 0, sizeof(KosDisplayInfo));

    sp<IBinder> mainDpy = SurfaceComposerClient::getBuiltInDisplay(
            ISurfaceComposer::eDisplayIdMain);
    DisplayInfo mainDpyInfo;
    status_t err = SurfaceComposerClient::getDisplayInfo(mainDpy, &mainDpyInfo);
    if (err != NO_ERROR) {
        ALOGE("ERROR: kosGetDisplayInfo, unable to get display characteristics");
        return;
    }

    info->w = mainDpyInfo.w;
    info->h = mainDpyInfo.h;
    info->xdpi = mainDpyInfo.xdpi;
    info->ydpi = mainDpyInfo.ydpi;
    info->fps = mainDpyInfo.fps;
    info->density = mainDpyInfo.density;
    info->orientation = mainDpyInfo.orientation;
    info->secure = mainDpyInfo.secure;
}

static NativeConnection* gConnectionPtr = nullptr;

NDK_EXPORT bool kosCreateInput(bool keyboard, int screen_width, int screen_height)
{
    if (gConnectionPtr != nullptr) {
        ALOGD("kosCreateInput fail, at most one input device anytime");
        return false;
    }
    gConnectionPtr = NativeConnection::open("RDP uinput", "com.kos.launcher", keyboard, screen_width, screen_height);
    return gConnectionPtr != nullptr;
}

NDK_EXPORT void kosDestroyInput()
{
    if (gConnectionPtr == nullptr) {
        return;
    }
    delete gConnectionPtr;
    gConnectionPtr = nullptr;
}

NDK_EXPORT uint32_t kosSendInput(uint32_t input_count, KosInput* inputs)
{
    if (gConnectionPtr == nullptr) {
        return 0;
    }
    if (input_count == 0 || inputs == nullptr) {
        return 0;
    }
    return gConnectionPtr->send_input(input_count, inputs);
}

NDK_EXPORT void kosGetVersion(char* ver, int /*max_bytes*/)
{
    strcpy(ver, "1.0.3-20210422");
    // char msg[64];
    // kosNetGetCfg(msg, sizeof(msg));
}