/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef _NETDLISTENER_H__
#define _NETDLISTENER_H__

#include <map>
#include <sysutils/FrameworkListener.h>
#include <utils/Errors.h>
#include <kosapi/net.h>
#include "webrtc/rtc_base/event.h"

class NetdListener : public SocketListener {
public:
    static const int SendMsgFailCode = -1;
    static const int CMD_ARGS_MAX = 26;
    static const int CMDNUM_MIN = 1;
    static const int CMDNUM_MAX = INT_MAX;
    int mCmdNum;

    NetdListener(int sock);
    virtual ~NetdListener();

    static void dumpArgs(int argc, char **argv, int argObscure);
    static int sendGenericOkFail(SocketClient *cli, int cond);

    int sendMsg(const char* msg, char* resp, int maxBytes);
    void setNetReceiveBroadcast(fkosNetReceiveBroadcast did, void* user) {
        mNetReceiveBroadcast = did;
        mNetReceiveBroadcastUser = user;
    }

protected:
    virtual bool onDataAvailable(SocketClient *c);
    void dispatchCommand(SocketClient *cli, char *data);
    void runCommand(int argc, char ** argv);

private:
    bool mSkipToNextNullByte;
    bool mWithSeq;
    fkosNetReceiveBroadcast mNetReceiveBroadcast;
    void* mNetReceiveBroadcastUser;
    rtc::Event mRespEvent;

    struct tslot {
        tslot(char* _result, int _maxBytes)
            : result(_result)
            , maxBytes(_maxBytes)
            , code(0)
        {}

        char* result;
        int maxBytes;
        int code;
    };
    std::map<int, tslot>    mSlots;
    pthread_mutex_t         mSlotsLock;
};

#endif
