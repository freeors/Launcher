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

#include <stdlib.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
// #include <fs_mgr.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>

#define LOG_TAG "NetdListener"

// #include <android-base/logging.h>
// #include <android-base/stringprintf.h>
#include <cutils/fs.h>
#include <utils/Log.h>
#include <cutils/sockets.h>

#include <sysutils/SocketClient.h>
#include <private/android_filesystem_config.h>
#include <string>
#include <vector>

#include "NetdListener.h"
#include "NetdResponseCode.h"
#include <kosapi/net.h>

static const int CMD_BUF_SIZE = 1024;


NetdListener::NetdListener(int sock) :
                 SocketListener(sock, false)
                 , mCmdNum(CMDNUM_MIN)
                 , mSkipToNextNullByte(false)
                 , mWithSeq(false)
                 , mNetReceiveBroadcast(nullptr)
                 , mNetReceiveBroadcastUser(nullptr)
                 , mRespEvent(true, false)
{
    ALOGD("------NetdListener::NetdListener------, sock: %i, this: 0x%p", sock, this);
    pthread_mutex_init(&mSlotsLock, NULL);
}

NetdListener::~NetdListener()
{
    ALOGD("------NetdListener::~NetdListener------ this: 0x%p", this);
}

bool NetdListener::onDataAvailable(SocketClient *c)
{
    char buffer[CMD_BUF_SIZE];
    int len;

    len = TEMP_FAILURE_RETRY(read(c->getSocket(), buffer, sizeof(buffer)));
    ALOGD("NetdListener::onDataAvailable, len: %i", len);
    if (len < 0) {
        SLOGE("read() failed (%s)", strerror(errno));
        return false;
    } else if (!len) {
        return false;
    } else if (buffer[len-1] != '\0') {
        SLOGW("String is not zero-terminated");
        android_errorWriteLog(0x534e4554, "29831647");
        c->sendMsg(500, "Command too large for buffer", false);
        mSkipToNextNullByte = true;
        return false;
    }

    int offset = 0;
    int i;

    for (i = 0; i < len; i++) {
        if (buffer[i] == '\0') {
            /* IMPORTANT: dispatchCommand() expects a zero-terminated string */
            if (mSkipToNextNullByte) {
                mSkipToNextNullByte = false;
            } else {
                dispatchCommand(c, buffer + offset);
            }
            offset = i + 1;
        }
    }

    mSkipToNextNullByte = false;
    return true;
}

void NetdListener::dispatchCommand(SocketClient *cli, char *data) {
    // FrameworkCommandCollection::iterator i;
    int argc = 0;
    char *argv[NetdListener::CMD_ARGS_MAX];
    char tmp[CMD_BUF_SIZE];
    char *p = data;
    char *q = tmp;
    char *qlimit = tmp + sizeof(tmp) - 1;
    bool esc = false;
    bool quote = false;
    bool haveCmdNum = !mWithSeq;

    ALOGD("dispatchCommand (%s)", data);

    memset(argv, 0, sizeof(argv));
    memset(tmp, 0, sizeof(tmp));
    while(*p) {
        if (*p == '\\') {
            if (esc) {
                if (q >= qlimit)
                    goto overflow;
                *q++ = '\\';
                esc = false;
            } else
                esc = true;
            p++;
            continue;
        } else if (esc) {
            if (*p == '"') {
                if (q >= qlimit)
                    goto overflow;
                *q++ = '"';
            } else if (*p == '\\') {
                if (q >= qlimit)
                    goto overflow;
                *q++ = '\\';
            } else {
                cli->sendMsg(500, "Unsupported escape sequence", false);
                goto out;
            }
            p++;
            esc = false;
            continue;
        }

        if (*p == '"') {
            if (quote)
                quote = false;
            else
                quote = true;
            p++;
            continue;
        }

        if (q >= qlimit)
            goto overflow;
        *q = *p++;
        if (!quote && *q == ' ') {
            *q = '\0';
            if (!haveCmdNum) {
                char *endptr;
                int cmdNum = (int)strtol(tmp, &endptr, 0);
                if (endptr == NULL || *endptr != '\0') {
                    cli->sendMsg(500, "Invalid sequence number", false);
                    goto out;
                }
                cli->setCmdNum(cmdNum);
                haveCmdNum = true;
            } else {
                if (argc >= CMD_ARGS_MAX)
                    goto overflow;
                argv[argc++] = strdup(tmp);
            }
            memset(tmp, 0, sizeof(tmp));
            q = tmp;
            continue;
        }
        q++;
    }

    *q = '\0';
    if (argc >= CMD_ARGS_MAX)
        goto overflow;
    argv[argc++] = strdup(tmp);
#if 0
    for (int k = 0; k < argc; k++) {
        SLOGD("arg[%d] = '%s'", k, argv[k]);
    }
#endif

    if (quote) {
        cli->sendMsg(500, "Unclosed quotes error", false);
        goto out;
    }
/*
    if (errorRate && (++mCommandCount % errorRate == 0)) {
        // ignore this command - let the timeout handler handle it
        SLOGE("Faking a timeout");
        goto out;
    }
*/
/*
    for (i = mCommands->begin(); i != mCommands->end(); ++i) {
        FrameworkCommand *c = *i;

        if (!strcmp(argv[0], c->getCommand())) {
            if (c->runCommand(cli, argc, argv)) {
                SLOGW("Handler '%s' error (%s)", c->getCommand(), strerror(errno));
            }
            goto out;
        }
    }
*/

    if (argc > 0) {
        runCommand(argc, argv);
        goto out;
    }

    cli->sendMsg(500, "Command not recognized", false);
out:
    int j;
    for (j = 0; j < argc; j++)
        free(argv[j]);
    return;

overflow:
    LOG_EVENT_INT(78001, cli->getUid());
    cli->sendMsg(500, "Command too long", false);
    goto out;
}

static const char *kUpdated = "updated";
static const char *kRemoved = "removed";

uint32_t splitIpPrefixlen(char* src, int* prefixlen)
{
    char* ptr = strchr(src, '/');
    if (ptr == nullptr) {
        return 0;
    }

    *ptr = '\0';
    struct in_addr addr;
    if (!inet_aton(src, &addr)) {
        return 0;
    }
    if (prefixlen != nullptr) {
        char *endptr;
        *prefixlen = (int)strtol(ptr + 1, &endptr, 0);
    }
    return addr.s_addr;
}

static int calcCmdNum(const char* c_str)
{
	const int len = strlen(c_str);
	if (len == 0) {
		return -1;
	}

    int ret = 0;
	for (int at = 0; at < len; at ++) {
	    const char ch = c_str[at];
		if (ch < '0' || ch > '9') {
			return -1;
		}
		ret = ret * 10 + ch - '0';
	}
	return ret;
}

void NetdListener::runCommand(int argc, char ** argv)
{
    if (argc <= 1) {
        ALOGD("runCommand, argc(%i) <= 1, think as noise", argc);
        return;
    }

    char *endptr;
    int code = (int)strtol(argv[0], &endptr, 0);

    // NetdResponseCode::InterfaceGetCfgResult)
    int cmdNum = calcCmdNum(argv[1]);
    ALOGD("runCommand, [0]code: %i, [1]cmd/cmdNum: (%s)/(%i)", code, argv[1], cmdNum);
    if (cmdNum < 0) {
        // broadcast
        if (mNetReceiveBroadcast != nullptr) {
            mNetReceiveBroadcast(argc, (const char**)argv, mNetReceiveBroadcastUser);
        }
        return;
    }
    pthread_mutex_lock(&mSlotsLock);
    tslot* slotPtr = nullptr;
    if (mSlots.count(cmdNum) != 0) {
        slotPtr = &(mSlots.find(cmdNum)->second);
    }
    if (slotPtr != nullptr) {
        slotPtr->code = code;
        int residue = slotPtr->maxBytes;
        char* ptr = slotPtr->result;
        for (int n = 2; n < argc; n ++) {
            int len = strlen(argv[n]);
            if (residue <= len) {
                break;
            }
            memcpy(ptr, argv[n], len);
            ptr[len] = ' ';
            ptr += len + 1;
            residue -= len + 1;
        }
        if (ptr != slotPtr->result) {
            ptr --;
            *ptr = '\0';
        }
        mRespEvent.Set();
    }
    pthread_mutex_unlock(&mSlotsLock);
}

int NetdListener::sendMsg(const char* msg, char* resp, int maxBytes)
{
    if (mClients->empty()) {
        ALOGD("NetdListener::sendMsg, mClients is empty");
        return SendMsgFailCode;
    }
    if (msg == nullptr || msg[0] == '\0') {
        ALOGD("NetdListener::sendMsg, msg must not empty");
        return SendMsgFailCode;
    }
    if (resp == nullptr || maxBytes <= 0) {
        ALOGD("NetdListener::sendMsg, resp must be valid");
        return SendMsgFailCode;
    }


    pthread_mutex_lock(&mSlotsLock);
    ALOGD("NetdListener::sendMsg, msg: %s, maxBytes: %i, mSlots: %i", msg, maxBytes, (int)mSlots.size());

    mSlots.insert(std::make_pair(mCmdNum, tslot(resp, maxBytes)));

    char* buf;
    asprintf(&buf, "%d %s", mCmdNum, msg);
    const int cmdNum = mCmdNum;
    mCmdNum = mCmdNum != CMDNUM_MAX? mCmdNum + 1: CMDNUM_MIN;
    pthread_mutex_unlock(&mSlotsLock);

    mRespEvent.Reset();
    pthread_mutex_lock(&mClientsLock);
    SocketClient* c = *(mClients->begin());
    c->sendMsg(buf);
    pthread_mutex_unlock(&mClientsLock);
    free(buf);

    const int overflowMs = 3000;
    if (!mRespEvent.Wait(overflowMs)) {
        ALOGD("NetdListener::sendMsg, msg: %s, post wait fail", msg);
        mRespEvent.Reset();
    }

    pthread_mutex_lock(&mSlotsLock);
    std::map<int, tslot>::iterator findIt = mSlots.find(cmdNum);
    const int code = findIt->second.code;
    mSlots.erase(findIt);
    pthread_mutex_unlock(&mSlotsLock);
    return code;
}

static NetdListener* netdl = nullptr;

static NetdListener* makeSureNetdListener()
{
    // ALOGD("[NetdListener.cpp]makeSureNetdListener, netdl: 0x%p", netdl);
    if (netdl != nullptr) {
        return netdl;
    }
    std::string socketName = "netd";
    int sock = socket_local_client(socketName.c_str(), ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM);
    ALOGD("[NetdListener.cpp]kosRunNetdListener, socket_local_client(%s, ...), sock: %i", socketName.c_str(), sock);
    if (sock <= 0) {
        return nullptr;
    }
    netdl = new NetdListener(sock);
    netdl->startListener();
    return netdl;
}

NDK_EXPORT void kosNetSetReceiveBroadcast(fkosNetReceiveBroadcast did, void* user)
{
    if (makeSureNetdListener() == nullptr) {
        ALOGE("kosNetSetReceiveBroadcast, netdl is nullptr");
        return;
    }
    netdl->setNetReceiveBroadcast(did, user);
}

NDK_EXPORT int kosNetSendMsg(const char* msg, char* result, int maxBytes)
{
    if (makeSureNetdListener() == nullptr) {
        return NetdListener::SendMsgFailCode;
    }
    return netdl->sendMsg(msg, result, maxBytes);
}
