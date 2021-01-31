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

#ifndef LIBKOSAPI_EVENTHUB_H
#define LIBKOSAPI_EVENTHUB_H

#include <input/Input.h>
#include <input/InputDevice.h>
#include <input/Keyboard.h>
#include <input/KeyLayoutMap.h>
#include <input/KeyCharacterMap.h>
#include <input/VirtualKeyMap.h>
#include <utils/String8.h>
#include <utils/threads.h>
#include <utils/Log.h>
#include <utils/threads.h>
#include <utils/List.h>
#include <utils/Errors.h>
#include <utils/PropertyMap.h>
#include <utils/Vector.h>
#include <utils/KeyedVector.h>
#include <utils/BitSet.h>

#include <linux/input.h>
#include <sys/epoll.h>
#include <kosapi/sys.h>

namespace android {

enum {
    // Device id of a special "virtual" keyboard that is always present.
    VIRTUAL_KEYBOARD_ID = -1,
    // Device id of the "built-in" keyboard if there is one.
    BUILT_IN_KEYBOARD_ID = 0,
};

/*
 * Input device classes.
 */
enum {
    /* The input device is a keyboard or has buttons. */
    INPUT_DEVICE_CLASS_KEYBOARD      = 0x00000001,

    /* The input device is an alpha-numeric keyboard (not just a dial pad). */
    INPUT_DEVICE_CLASS_ALPHAKEY      = 0x00000002,

    /* The input device is a touchscreen or a touchpad (either single-touch or multi-touch). */
    INPUT_DEVICE_CLASS_TOUCH         = 0x00000004,

    /* The input device is a cursor device such as a trackball or mouse. */
    INPUT_DEVICE_CLASS_CURSOR        = 0x00000008,

    /* The input device is a multi-touch touchscreen. */
    INPUT_DEVICE_CLASS_TOUCH_MT      = 0x00000010,

    /* The input device is a directional pad (implies keyboard, has DPAD keys). */
    INPUT_DEVICE_CLASS_DPAD          = 0x00000020,

    /* The input device is a gamepad (implies keyboard, has BUTTON keys). */
    INPUT_DEVICE_CLASS_GAMEPAD       = 0x00000040,

    /* The input device has switches. */
    INPUT_DEVICE_CLASS_SWITCH        = 0x00000080,

    /* The input device is a joystick (implies gamepad, has joystick absolute axes). */
    INPUT_DEVICE_CLASS_JOYSTICK      = 0x00000100,

    /* The input device has a vibrator (supports FF_RUMBLE). */
    INPUT_DEVICE_CLASS_VIBRATOR      = 0x00000200,
    /* The input device has a mouse. */
    INPUT_DEVICE_CLASS_KEYMOUSE      = 0x00000400,

    /* The input device has a microphone. */
    INPUT_DEVICE_CLASS_MIC           = 0x00000400,

    /* The input device is an external stylus (has data we want to fuse with touch data). */
    INPUT_DEVICE_CLASS_EXTERNAL_STYLUS = 0x00000800,

    /* The input device has a rotary encoder */
    INPUT_DEVICE_CLASS_ROTARY_ENCODER = 0x00001000,

    /* The input device is virtual (not a real device, not part of UI configuration). */
    INPUT_DEVICE_CLASS_VIRTUAL       = 0x40000000,

    /* The input device is external (not built-in). */
    INPUT_DEVICE_CLASS_EXTERNAL      = 0x80000000,
};

/*
 * Gets the class that owns an axis, in cases where multiple classes might claim
 * the same axis for different purposes.
 */
extern uint32_t getAbsAxisUsage(int32_t axis, uint32_t deviceClasses);

class EventHub
{
public:
    EventHub();
    virtual ~EventHub();

    void start();
    void loopOnce();
    // uint32_t send_input(uint32_t input_count, kosInput* inputs, int sizeof_input);

private:
    struct Device {
        Device* next;

        int fd; // may be -1 if device is virtual
        const int32_t id;
        const String8 path;
        const InputDeviceIdentifier identifier;

        uint32_t classes;

        uint8_t keyBitmask[(KEY_MAX + 1) / 8];
        uint8_t absBitmask[(ABS_MAX + 1) / 8];
        uint8_t relBitmask[(REL_MAX + 1) / 8];
        uint8_t swBitmask[(SW_MAX + 1) / 8];
        uint8_t ledBitmask[(LED_MAX + 1) / 8];
        uint8_t ffBitmask[(FF_MAX + 1) / 8];
        uint8_t propBitmask[(INPUT_PROP_MAX + 1) / 8];

        String8 configurationFile;
        PropertyMap* configuration;
        VirtualKeyMap* virtualKeyMap;
        KeyMap keyMap;

        sp<KeyCharacterMap> overlayKeyMap;
        sp<KeyCharacterMap> combinedKeyMap;

        bool ffEffectPlaying;
        int16_t ffEffectId; // initially -1

        int32_t controllerNumber;

        int32_t timestampOverrideSec;
        int32_t timestampOverrideUsec;

        Device(int fd, int32_t id, const String8& path, const InputDeviceIdentifier& identifier);
        ~Device();

        void close();

        inline bool isVirtual() const { return fd < 0; }

        const sp<KeyCharacterMap>& getKeyCharacterMap() const {
            if (combinedKeyMap != NULL) {
                return combinedKeyMap;
            }
            return keyMap.keyCharacterMap;
        }
    };

    status_t openDeviceLocked(const char *devicePath);
    void addDeviceLocked(Device* device);

    void closeDeviceLocked(Device* device);
    void closeAllDevicesLocked();

    status_t scanDirLocked(const char *dirname);
    void scanDevicesLocked();
    status_t readNotifyLocked();

    void assignDescriptorLocked(InputDeviceIdentifier& identifier);
    status_t loadVirtualKeyMapLocked(Device* device);
    status_t loadKeyMapLocked(Device* device);
    bool isExternalDeviceLocked(Device* device);
    bool hasKeycodeLocked(Device* device, int keycode) const;
    bool deviceHasMicLocked(Device* device);
    int32_t getNextControllerNumberLocked(Device* device);
    void releaseControllerNumberLocked(Device* device);
    Device* getDeviceByDescriptorLocked(String8& descriptor) const;
    status_t closeDeviceByPathLocked(const char *devicePath);
    Device* getDeviceLocked(int32_t deviceId) const;
    Device* getDeviceByPathLocked(const char* devicePath) const;

private:
    // Protect all internal state.
    mutable Mutex mLock;

    // The actual id of the built-in keyboard, or NO_BUILT_IN_KEYBOARD if none.
    // EventHub remaps the built-in keyboard to id 0 externally as required by the API.
    enum {
        // Must not conflict with any other assigned device ids, including
        // the virtual keyboard id (-1).
        NO_BUILT_IN_KEYBOARD = -2,
    };
    int32_t mBuiltInKeyboardId;

    int32_t mNextDeviceId;

    BitSet32 mControllerNumbers;

    KeyedVector<int32_t, Device*> mDevices;

    int mEpollFd;
    int mINotifyFd;

    // Ids used for epoll notifications not associated with devices.
    static const uint32_t EPOLL_ID_INOTIFY = 0x80000001;
    static const uint32_t EPOLL_ID_WAKE = 0x80000002;

    // Epoll FD list size hint.
    static const int EPOLL_SIZE_HINT = 8;

    // Maximum number of signalled FDs to handle at a time.
    static const int EPOLL_MAX_EVENTS = 16;

    // The array of pending epoll events and the index of the next event to be handled.
    struct epoll_event mPendingEventItems[EPOLL_MAX_EVENTS];
    size_t mPendingEventCount;
    size_t mPendingEventIndex;
    bool mPendingINotify;
};

}; // namespace android

#endif /*LIBKOSAPI_EVENTHUB_h*/
