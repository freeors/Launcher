/*
 * Copyright (C) 2011 The Android Open Source Project
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

#define LOG_TAG "InputEventReceiver"

//#define LOG_NDEBUG 0

#include "InputEventReceiver.h"

#include <android_runtime/AndroidRuntime.h>
#include <utils/Log.h>
#include <utils/Looper.h>
#include <utils/Vector.h>
#include <utils/threads.h>
#include <input/InputTransport.h>

#include <ScopedLocalRef.h>

namespace android {

static const bool kDebugDispatchCycle = false;

NativeInputEventReceiver::NativeInputEventReceiver(const sp<InputChannel>& inputChannel)
        : mInputConsumer(inputChannel)
        , mLooper(new Looper(false))
        , mBatchedInputEventPending(false)
        , mFdEvents(0)
        , did_touched_(NULL) {
    if (kDebugDispatchCycle) {
        ALOGD("channel '%s' ~ Initializing input event receiver.", getInputChannelName());
    }
}

NativeInputEventReceiver::~NativeInputEventReceiver() {
}

status_t NativeInputEventReceiver::initialize() {
    setFdEvents(ALOOPER_EVENT_INPUT);
    return OK;
}

void NativeInputEventReceiver::set_did(fdid_sys2_touched did_touched, fdid_sys2_hover_moved did_hover_moved)
{
    did_touched_ = did_touched;
    did_hover_moved_ = did_hover_moved;
}

void NativeInputEventReceiver::dispose() {
    if (kDebugDispatchCycle) {
        ALOGD("channel '%s' ~ Disposing input event receiver.", getInputChannelName());
    }

    setFdEvents(0);
}

status_t NativeInputEventReceiver::finishInputEvent(uint32_t seq, bool handled) {
    if (kDebugDispatchCycle) {
        ALOGD("channel '%s' ~ Finished input event.", getInputChannelName());
    }

    status_t status = mInputConsumer.sendFinishedSignal(seq, handled);
    if (status) {
        if (status == WOULD_BLOCK) {
            if (kDebugDispatchCycle) {
                ALOGD("channel '%s' ~ Could not send finished signal immediately.  "
                        "Enqueued for later.", getInputChannelName());
            }
            Finish finish;
            finish.seq = seq;
            finish.handled = handled;
            mFinishQueue.add(finish);
            if (mFinishQueue.size() == 1) {
                setFdEvents(ALOOPER_EVENT_INPUT | ALOOPER_EVENT_OUTPUT);
            }
            return OK;
        }
        ALOGW("Failed to send finished signal on channel '%s'.  status=%d",
                getInputChannelName(), status);
    }
    return status;
}

void NativeInputEventReceiver::setFdEvents(int events) {
    if (mFdEvents != events) {
        mFdEvents = events;
        int fd = mInputConsumer.getChannel()->getFd();
        if (events) {
            mLooper->addFd(fd, 0, events, this, NULL);
        } else {
            mLooper->removeFd(fd);
        }
    }
}

int NativeInputEventReceiver::handleEvent(int /*receiveFd*/, int events, void* /*data*/) {
    if (events & (ALOOPER_EVENT_ERROR | ALOOPER_EVENT_HANGUP)) {
        // This error typically occurs when the publisher has closed the input channel
        // as part of removing a window or finishing an IME session, in which case
        // the consumer will soon be disposed as well.
        if (kDebugDispatchCycle) {
            ALOGD("channel '%s' ~ Publisher closed input channel or an error occurred.  "
                    "events=0x%x", getInputChannelName(), events);
        }
        return 0; // remove the callback
    }

    if (events & ALOOPER_EVENT_INPUT) {
        // set consumeBatches to true,  I hope not to use batch.
        status_t status = consumeEvents(true, -1, NULL);
        return status == OK || status == NO_MEMORY ? 1 : 0;
    }

    if (events & ALOOPER_EVENT_OUTPUT) {
        for (size_t i = 0; i < mFinishQueue.size(); i++) {
            const Finish& finish = mFinishQueue.itemAt(i);
            status_t status = mInputConsumer.sendFinishedSignal(finish.seq, finish.handled);
            if (status) {
                mFinishQueue.removeItemsAt(0, i);

                if (status == WOULD_BLOCK) {
                    if (kDebugDispatchCycle) {
                        ALOGD("channel '%s' ~ Sent %zu queued finish events; %zu left.",
                                getInputChannelName(), i, mFinishQueue.size());
                    }
                    return 1; // keep the callback, try again later
                }

                ALOGW("Failed to send finished signal on channel '%s'.  status=%d",
                        getInputChannelName(), status);
                if (status != DEAD_OBJECT) {
                    ALOGD("Failed to finish input event.  status=%d", status);
                }
                return 0; // remove the callback
            }
        }
        if (kDebugDispatchCycle) {
            ALOGD("channel '%s' ~ Sent %zu queued finish events; none left.",
                    getInputChannelName(), mFinishQueue.size());
        }
        mFinishQueue.clear();
        setFdEvents(ALOOPER_EVENT_INPUT);
        return 1;
    }

    ALOGW("channel '%s' ~ Received spurious callback for unhandled poll event.  "
            "events=0x%x", getInputChannelName(), events);
    return 1;
}

status_t NativeInputEventReceiver::consumeEvents(bool consumeBatches, nsecs_t frameTime, bool* outConsumedBatch) {
    if (kDebugDispatchCycle) {
        ALOGD("channel '%s' ~ Consuming input events, consumeBatches=%s, frameTime=%lld.",
                getInputChannelName(), consumeBatches ? "true" : "false", (long long)frameTime);
    }

    if (consumeBatches) {
        mBatchedInputEventPending = false;
    }
    if (outConsumedBatch) {
        *outConsumedBatch = false;
    }

    // void* receiverObj = NULL;
    bool skipCallbacks = false;
    for (;;) {
        uint32_t seq;
        InputEvent* inputEvent;
        status_t status = mInputConsumer.consume(&mInputEventFactory,
                consumeBatches, frameTime, &seq, &inputEvent);
        if (status) {
            if (status == WOULD_BLOCK) {
                // normal. have read all message.
                return OK;
            }
            ALOGE("channel '%s' ~ Failed to consume input event.  status=%d",
                    getInputChannelName(), status);
            return status;
        }
        assert(inputEvent);

        if (!skipCallbacks) {
/*
            if (!receiverObj.get()) {
                ALOGW("channel '%s' ~ Receiver object was finalized "
                        "without being disposed.", getInputChannelName());
                return DEAD_OBJECT;
            }
*/
            void* inputEventObj = NULL;
            switch (inputEvent->getType()) {
            case AINPUT_EVENT_TYPE_KEY:
                if (kDebugDispatchCycle) {
                    ALOGD("channel '%s' ~ Received key event.", getInputChannelName());
                }
                // inputEventObj = ;
                break;

            case AINPUT_EVENT_TYPE_MOTION: {
                MotionEvent* motionEvent = static_cast<MotionEvent*>(inputEvent);
                if ((motionEvent->getAction() & AMOTION_EVENT_ACTION_MOVE) && outConsumedBatch) {
                    *outConsumedBatch = true;
                }

                int actionMasked = motionEvent->getActionMasked();
                int actionIndex = motionEvent->getActionIndex();
                float x = motionEvent->getX(0);
                float y = motionEvent->getY(0);
                if (kDebugDispatchCycle) {
                    ALOGD("channel '%s' ~ Received motion event. deviceId: %i, source: %i, actionMasked: %i, actionIndex: %i, (%f, %f)", getInputChannelName(), motionEvent->getDeviceId(), motionEvent->getSource(), actionMasked, actionIndex, x, y);
                }
                if (actionMasked == AMOTION_EVENT_ACTION_DOWN || actionMasked == AMOTION_EVENT_ACTION_UP || actionMasked == AMOTION_EVENT_ACTION_MOVE) {
                    if (did_touched_) {
                        did_touched_(motionEvent->getDeviceId(), actionIndex, actionMasked, x / 1920, y / 1080, 0);
                    }
                }
                if (actionMasked == AMOTION_EVENT_ACTION_HOVER_MOVE) {
                    if (did_hover_moved_) {
                        did_hover_moved_(0, actionMasked, x, y);
                    }
                }
                // inputEventObj = ;
                break;
            }

            default:
                assert(false); // InputConsumer should prevent this from ever happening
                inputEventObj = NULL;
            }

            // if (inputEventObj) {
            if (true) {
                if (kDebugDispatchCycle) {
                    ALOGD("channel '%s' ~ Dispatching input event.", getInputChannelName());
                }
                bool ret = true;
                // env->CallVoidMethod(receiverObj.get(), gInputEventReceiverClassInfo.dispatchInputEvent, seq, inputEventObj);
                finishInputEvent(seq, true);
                if (!ret) {
                    ALOGE("Exception dispatching input event.");
                    skipCallbacks = true;
                }
            } else {
                ALOGW("channel '%s' ~ Failed to obtain event object.", getInputChannelName());
                skipCallbacks = true;
            }
        }

        if (skipCallbacks) {
            mInputConsumer.sendFinishedSignal(seq, false);
        }
    }
}

/*
static void nativeDispose(JNIEnv* env, jclass clazz, jlong receiverPtr) {
    sp<NativeInputEventReceiver> receiver =
            reinterpret_cast<NativeInputEventReceiver*>(receiverPtr);
    receiver->dispose();
    receiver->decStrong(gInputEventReceiverClassInfo.clazz); // drop reference held by the object
}

static void nativeFinishInputEvent(JNIEnv* env, jclass clazz, jlong receiverPtr,
        jint seq, jboolean handled) {
    sp<NativeInputEventReceiver> receiver =
            reinterpret_cast<NativeInputEventReceiver*>(receiverPtr);
    status_t status = receiver->finishInputEvent(seq, handled);
    if (status && status != DEAD_OBJECT) {
        String8 message;
        message.appendFormat("Failed to finish input event.  status=%d", status);
        jniThrowRuntimeException(env, message.string());
    }
}

static jboolean nativeConsumeBatchedInputEvents(JNIEnv* env, jclass clazz, jlong receiverPtr,
        jlong frameTimeNanos) {
    sp<NativeInputEventReceiver> receiver =
            reinterpret_cast<NativeInputEventReceiver*>(receiverPtr);
    bool consumedBatch;
    status_t status = receiver->consumeEvents(env, true, frameTimeNanos,
            &consumedBatch);
    if (status && status != DEAD_OBJECT && !env->ExceptionCheck()) {
        String8 message;
        message.appendFormat("Failed to consume batched input event.  status=%d", status);
        jniThrowRuntimeException(env, message.string());
        return JNI_FALSE;
    }
    return consumedBatch ? JNI_TRUE : JNI_FALSE;
}
*/
} // namespace android
