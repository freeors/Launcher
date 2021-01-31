#ifndef _LIBKOSAPI_NET_H
#define _LIBKOSAPI_NET_H

#ifndef _WIN32
#include <sys/cdefs.h>
#include <stdint.h>
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// fkosNetStateChanged will run speical thread.
typedef void (*fkosNetReceiveBroadcast)(int argc, const char** argv, void* user);
void kosNetSetReceiveBroadcast(fkosNetReceiveBroadcast did, void* user);

// max_bytes require include terminate char: '\0'
int kosNetSendMsg(const char* msg, char* result, int maxBytes);

#ifdef __cplusplus
}
#endif

#endif /* _LIBKOSAPI_NET_H */
