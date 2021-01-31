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

#ifndef ANDROID_SYSTEM_SERVER_INPUTEVENTRECEIVER_H
#define ANDROID_SYSTEM_SERVER_INPUTEVENTRECEIVER_H

#include <android_runtime/AndroidRuntime.h>
#include <utils/Log.h>
#include <utils/Looper.h>
#include <utils/Vector.h>
#include <utils/threads.h>
#include <input/InputTransport.h>

#include <ScopedLocalRef.h>
#include <kosapi/sys2.h>

namespace android {

class NativeInputEventReceiver : public LooperCallback {
public:
    NativeInputEventReceiver(const sp<InputChannel>& inputChannel);

    status_t initialize();
    void set_did(fdid_sys2_touched did_touched, fdid_sys2_hover_moved did_hover_moved);
    void dispose();
    status_t finishInputEvent(uint32_t seq, bool handled);
    status_t consumeEvents(bool consumeBatches, nsecs_t frameTime,
            bool* outConsumedBatch);
    void pollOnce() {
        mLooper->pollOnce(0);
    }

protected:
    virtual ~NativeInputEventReceiver();

private:
    void setFdEvents(int events);

    const char* getInputChannelName() {
        return mInputConsumer.getChannel()->getName().string();
    }

    virtual int handleEvent(int receiveFd, int events, void* data);

private:
    struct Finish {
        uint32_t seq;
        bool handled;
    };

    InputConsumer mInputConsumer;
    sp<Looper> mLooper;
    PreallocatedInputEventFactory mInputEventFactory;
    bool mBatchedInputEventPending;
    int mFdEvents;
    Vector<Finish> mFinishQueue;
    fdid_sys2_touched did_touched_;
    fdid_sys2_hover_moved did_hover_moved_;
};

} // namespace android

#endif // ANDROID_SYSTEM_SERVER_INPUTEVENTRECEIVER_H