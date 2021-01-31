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
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/limits.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <unistd.h>

#define LOG_TAG "libkosapi-eventhub"

#define LOG_NDEBUG 0

#include "eventhub.h"

#include <cutils/properties.h>
#include <openssl/sha.h>

#include <input/KeyLayoutMap.h>
#include <input/KeyCharacterMap.h>
#include <input/VirtualKeyMap.h>

/* this macro is used to tell if "bit" is set in "array"
 * it selects a byte from the array, and does a boolean AND
 * operation with a byte that only has the relevant bit set.
 * eg. to check for the 12th bit, we do (array[1] & 1<<4)
 */
#define test_bit(bit, array)    (array[bit/8] & (1<<(bit%8)))

/* this macro computes the number of bytes needed to represent a bit array of the specified size */
#define sizeof_bit_array(bits)  ((bits + 7) / 8)

using namespace android;

static const char *DEVICE_PATH = "/dev/input";

/* return the larger integer */
static inline int max(int v1, int v2)
{
    return (v1 > v2) ? v1 : v2;
}

static inline const char* toString(bool value) {
    return value ? "true" : "false";
}

static String8 sha1(const String8& in) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, reinterpret_cast<const u_char*>(in.string()), in.size());
    u_char digest[SHA_DIGEST_LENGTH];
    SHA1_Final(digest, &ctx);

    String8 out;
    for (size_t i = 0; i < SHA_DIGEST_LENGTH; i++) {
        out.appendFormat("%02x", digest[i]);
    }
    return out;
}

// --- Global Functions ---

uint32_t getAbsAxisUsage(int32_t axis, uint32_t deviceClasses) {
    // Touch devices get dibs on touch-related axes.
    if (deviceClasses & INPUT_DEVICE_CLASS_TOUCH) {
        switch (axis) {
        case ABS_X:
        case ABS_Y:
        case ABS_PRESSURE:
        case ABS_TOOL_WIDTH:
        case ABS_DISTANCE:
        case ABS_TILT_X:
        case ABS_TILT_Y:
        case ABS_MT_SLOT:
        case ABS_MT_TOUCH_MAJOR:
        case ABS_MT_TOUCH_MINOR:
        case ABS_MT_WIDTH_MAJOR:
        case ABS_MT_WIDTH_MINOR:
        case ABS_MT_ORIENTATION:
        case ABS_MT_POSITION_X:
        case ABS_MT_POSITION_Y:
        case ABS_MT_TOOL_TYPE:
        case ABS_MT_BLOB_ID:
        case ABS_MT_TRACKING_ID:
        case ABS_MT_PRESSURE:
        case ABS_MT_DISTANCE:
            return INPUT_DEVICE_CLASS_TOUCH;
        }
    }

    // External stylus gets the pressure axis
    if (deviceClasses & INPUT_DEVICE_CLASS_EXTERNAL_STYLUS) {
        if (axis == ABS_PRESSURE) {
            return INPUT_DEVICE_CLASS_EXTERNAL_STYLUS;
        }
    }

    // Joystick devices get the rest.
    return deviceClasses & INPUT_DEVICE_CLASS_JOYSTICK;
}

// --- EventHub::Device ---

EventHub::Device::Device(int fd, int32_t id, const String8& path,
        const InputDeviceIdentifier& identifier) :
        next(NULL),
        fd(fd), id(id), path(path), identifier(identifier),
        classes(0), configuration(NULL), virtualKeyMap(NULL),
        ffEffectPlaying(false), ffEffectId(-1), controllerNumber(0),
        timestampOverrideSec(0), timestampOverrideUsec(0) {
    ALOGD("EventHub::Device::Device--- id: %i", id);
    memset(keyBitmask, 0, sizeof(keyBitmask));
    memset(absBitmask, 0, sizeof(absBitmask));
    memset(relBitmask, 0, sizeof(relBitmask));
    memset(swBitmask, 0, sizeof(swBitmask));
    memset(ledBitmask, 0, sizeof(ledBitmask));
    memset(ffBitmask, 0, sizeof(ffBitmask));
    memset(propBitmask, 0, sizeof(propBitmask));
    ALOGD("---EventHub::Device::Device X id: %i", id);
}

EventHub::Device::~Device() {
    ALOGD("EventHub::Device::~Device--- id: %i", id);
    close();
    delete configuration;
    delete virtualKeyMap;
    ALOGD("---EventHub::Device::~Device X id: %i", id);
}

void EventHub::Device::close() {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}


// --- EventHub ---

const uint32_t EventHub::EPOLL_ID_INOTIFY;
const int EventHub::EPOLL_SIZE_HINT;
const int EventHub::EPOLL_MAX_EVENTS;

EventHub::EventHub(void) :
        mBuiltInKeyboardId(NO_BUILT_IN_KEYBOARD), mNextDeviceId(1), mControllerNumbers(),
        mPendingEventCount(0), mPendingEventIndex(0), mPendingINotify(false)
{
    ALOGD("EventHub::EventHub---");
    mEpollFd = epoll_create(EPOLL_SIZE_HINT);
    LOG_ALWAYS_FATAL_IF(mEpollFd < 0, "Could not create epoll instance.  errno=%d", errno);

    mINotifyFd = inotify_init();
    int result = inotify_add_watch(mINotifyFd, DEVICE_PATH, IN_DELETE | IN_CREATE);
    LOG_ALWAYS_FATAL_IF(result < 0, "Could not register INotify for %s.  errno=%d",
            DEVICE_PATH, errno);

    struct epoll_event eventItem;
    memset(&eventItem, 0, sizeof(eventItem));
    eventItem.events = EPOLLIN;
    eventItem.data.u32 = EPOLL_ID_INOTIFY;
    result = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mINotifyFd, &eventItem);
    LOG_ALWAYS_FATAL_IF(result != 0, "Could not add INotify to epoll instance.  errno=%d", errno);
    ALOGD("---EventHub::EventHub X");
}

EventHub::~EventHub(void) {
    ALOGD("EventHub::~EventHub---");
    closeAllDevicesLocked();

    ::close(mEpollFd);
    ::close(mINotifyFd);

    // release_wake_lock(WAKE_LOCK_ID);
    ALOGD("---EventHub::~EventHub X");
}

// ----------------------------------------------------------------------------

static bool containsNonZeroByte(const uint8_t* array, uint32_t startIndex, uint32_t endIndex) {
    const uint8_t* end = array + endIndex;
    array += startIndex;
    while (array != end) {
        if (*(array++) != 0) {
            return true;
        }
    }
    return false;
}

static const int32_t GAMEPAD_KEYCODES[] = {
        AKEYCODE_BUTTON_A, AKEYCODE_BUTTON_B, AKEYCODE_BUTTON_C,
        AKEYCODE_BUTTON_X, AKEYCODE_BUTTON_Y, AKEYCODE_BUTTON_Z,
        AKEYCODE_BUTTON_L1, AKEYCODE_BUTTON_R1,
        AKEYCODE_BUTTON_L2, AKEYCODE_BUTTON_R2,
        AKEYCODE_BUTTON_THUMBL, AKEYCODE_BUTTON_THUMBR,
        AKEYCODE_BUTTON_START, AKEYCODE_BUTTON_SELECT, AKEYCODE_BUTTON_MODE,
};

status_t EventHub::openDeviceLocked(const char *devicePath) {
    char buffer[80];

    ALOGV("Opening device: %s", devicePath);

    int fd = open(devicePath, O_RDWR | O_CLOEXEC);
    if(fd < 0) {
        ALOGE("could not open %s, %s\n", devicePath, strerror(errno));
        return -1;
    }

    InputDeviceIdentifier identifier;

    // Get device name.
    if(ioctl(fd, EVIOCGNAME(sizeof(buffer) - 1), &buffer) < 1) {
        //fprintf(stderr, "could not get device name for %s, %s\n", devicePath, strerror(errno));
    } else {
        buffer[sizeof(buffer) - 1] = '\0';
        identifier.name.setTo(buffer);
    }
/*
    // Check to see if the device is on our excluded list
    for (size_t i = 0; i < mExcludedDevices.size(); i++) {
        const String8& item = mExcludedDevices.itemAt(i);
        if (identifier.name == item) {
            ALOGI("ignoring event id %s driver %s\n", devicePath, item.string());
            close(fd);
            return -1;
        }
    }
*/
    // Get device driver version.
    int driverVersion;
    if(ioctl(fd, EVIOCGVERSION, &driverVersion)) {
        ALOGE("could not get driver version for %s, %s\n", devicePath, strerror(errno));
        close(fd);
        return -1;
    }

    // Get device identifier.
    struct input_id inputId;
    if(ioctl(fd, EVIOCGID, &inputId)) {
        ALOGE("could not get device input id for %s, %s\n", devicePath, strerror(errno));
        close(fd);
        return -1;
    }
    identifier.bus = inputId.bustype;
    identifier.product = inputId.product;
    identifier.vendor = inputId.vendor;
    identifier.version = inputId.version;

    // Get device physical location.
    if(ioctl(fd, EVIOCGPHYS(sizeof(buffer) - 1), &buffer) < 1) {
        //fprintf(stderr, "could not get location for %s, %s\n", devicePath, strerror(errno));
    } else {
        buffer[sizeof(buffer) - 1] = '\0';
        identifier.location.setTo(buffer);
    }

    // Get device unique id.
    if(ioctl(fd, EVIOCGUNIQ(sizeof(buffer) - 1), &buffer) < 1) {
        //fprintf(stderr, "could not get idstring for %s, %s\n", devicePath, strerror(errno));
    } else {
        buffer[sizeof(buffer) - 1] = '\0';
        identifier.uniqueId.setTo(buffer);
    }

    // Fill in the descriptor.
    assignDescriptorLocked(identifier);

    // Make file descriptor non-blocking for use with poll().
    if (fcntl(fd, F_SETFL, O_NONBLOCK)) {
        ALOGE("Error %d making device file descriptor non-blocking.", errno);
        close(fd);
        return -1;
    }

    // Allocate device.  (The device object takes ownership of the fd at this point.)
    int32_t deviceId = mNextDeviceId++;
    Device* device = new Device(fd, deviceId, String8(devicePath), identifier);

    ALOGV("add device %d: %s\n", deviceId, devicePath);
    ALOGV("  bus:        %04x\n"
         "  vendor      %04x\n"
         "  product     %04x\n"
         "  version     %04x\n",
        identifier.bus, identifier.vendor, identifier.product, identifier.version);
    ALOGV("  name:       \"%s\"\n", identifier.name.string());
    ALOGV("  location:   \"%s\"\n", identifier.location.string());
    ALOGV("  unique id:  \"%s\"\n", identifier.uniqueId.string());
    ALOGV("  descriptor: \"%s\"\n", identifier.descriptor.string());
    ALOGV("  driver:     v%d.%d.%d\n",
        driverVersion >> 16, (driverVersion >> 8) & 0xff, driverVersion & 0xff);

    // Load the configuration file for the device.
    // loadConfigurationLocked(device);

    // Figure out the kinds of events the device reports.
    ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(device->keyBitmask)), device->keyBitmask);
    ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(device->absBitmask)), device->absBitmask);
    ioctl(fd, EVIOCGBIT(EV_REL, sizeof(device->relBitmask)), device->relBitmask);
    ioctl(fd, EVIOCGBIT(EV_SW, sizeof(device->swBitmask)), device->swBitmask);
    ioctl(fd, EVIOCGBIT(EV_LED, sizeof(device->ledBitmask)), device->ledBitmask);
    ioctl(fd, EVIOCGBIT(EV_FF, sizeof(device->ffBitmask)), device->ffBitmask);
    ioctl(fd, EVIOCGPROP(sizeof(device->propBitmask)), device->propBitmask);

    // See if this is a keyboard.  Ignore everything in the button range except for
    // joystick and gamepad buttons which are handled like keyboards for the most part.
    bool haveKeyboardKeys = containsNonZeroByte(device->keyBitmask, 0, sizeof_bit_array(BTN_MISC))
            || containsNonZeroByte(device->keyBitmask, sizeof_bit_array(KEY_OK),
                    sizeof_bit_array(KEY_MAX + 1));
    bool haveGamepadButtons = containsNonZeroByte(device->keyBitmask, sizeof_bit_array(BTN_MISC),
                    sizeof_bit_array(BTN_MOUSE))
            || containsNonZeroByte(device->keyBitmask, sizeof_bit_array(BTN_JOYSTICK),
                    sizeof_bit_array(BTN_DIGI));
    if (haveKeyboardKeys || haveGamepadButtons) {
        device->classes |= INPUT_DEVICE_CLASS_KEYBOARD | INPUT_DEVICE_CLASS_KEYMOUSE;
    }

    // See if this is a cursor device such as a trackball or mouse.
    if (test_bit(BTN_MOUSE, device->keyBitmask)
            && test_bit(REL_X, device->relBitmask)
            && test_bit(REL_Y, device->relBitmask)) {
        device->classes |= INPUT_DEVICE_CLASS_CURSOR;
    }

    // See if this is a rotary encoder type device.
    String8 deviceType = String8();
    if (device->configuration &&
        device->configuration->tryGetProperty(String8("device.type"), deviceType)) {
            if (!deviceType.compare(String8("rotaryEncoder"))) {
                device->classes |= INPUT_DEVICE_CLASS_ROTARY_ENCODER;
            }
    }

    // See if this is a touch pad.
    // Is this a new modern multi-touch driver?
    if (test_bit(ABS_MT_POSITION_X, device->absBitmask)
            && test_bit(ABS_MT_POSITION_Y, device->absBitmask)) {
        // Some joysticks such as the PS3 controller report axes that conflict
        // with the ABS_MT range.  Try to confirm that the device really is
        // a touch screen.
        if (test_bit(BTN_TOUCH, device->keyBitmask) || !haveGamepadButtons) {
            device->classes |= INPUT_DEVICE_CLASS_TOUCH | INPUT_DEVICE_CLASS_TOUCH_MT;
        }
    // Is this an old style single-touch driver?
    } else if (test_bit(BTN_TOUCH, device->keyBitmask)
            && test_bit(ABS_X, device->absBitmask)
            && test_bit(ABS_Y, device->absBitmask)) {
        device->classes |= INPUT_DEVICE_CLASS_TOUCH;
    // Is this a BT stylus?
    } else if ((test_bit(ABS_PRESSURE, device->absBitmask) ||
                test_bit(BTN_TOUCH, device->keyBitmask))
            && !test_bit(ABS_X, device->absBitmask)
            && !test_bit(ABS_Y, device->absBitmask)) {
        device->classes |= INPUT_DEVICE_CLASS_EXTERNAL_STYLUS;
        // Keyboard will try to claim some of the buttons but we really want to reserve those so we
        // can fuse it with the touch screen data, so just take them back. Note this means an
        // external stylus cannot also be a keyboard device.
        device->classes &= ~INPUT_DEVICE_CLASS_KEYBOARD;
    }

    // See if this device is a joystick.
    // Assumes that joysticks always have gamepad buttons in order to distinguish them
    // from other devices such as accelerometers that also have absolute axes.
    if (haveGamepadButtons) {
/*
        uint32_t assumedClasses = device->classes | INPUT_DEVICE_CLASS_JOYSTICK;
        for (int i = 0; i <= ABS_MAX; i++) {
            if (test_bit(i, device->absBitmask)
                    && (getAbsAxisUsage(i, assumedClasses) & INPUT_DEVICE_CLASS_JOYSTICK)) {
                device->classes = assumedClasses;
                break;
            }
        }
*/
    }

    // Check whether this device has switches.
    for (int i = 0; i <= SW_MAX; i++) {
        if (test_bit(i, device->swBitmask)) {
            device->classes |= INPUT_DEVICE_CLASS_SWITCH;
            break;
        }
    }

    // Check whether this device supports the vibrator.
    if (test_bit(FF_RUMBLE, device->ffBitmask)) {
        device->classes |= INPUT_DEVICE_CLASS_VIBRATOR;
    }

    // Configure virtual keys.
    if ((device->classes & INPUT_DEVICE_CLASS_TOUCH)) {
        // Load the virtual keys for the touch screen, if any.
        // We do this now so that we can make sure to load the keymap if necessary.
        status_t status = loadVirtualKeyMapLocked(device);
        if (!status) {
            device->classes |= INPUT_DEVICE_CLASS_KEYBOARD;
        }
    }

    // Load the key map.
    // We need to do this for joysticks too because the key layout may specify axes.
    status_t keyMapStatus = NAME_NOT_FOUND;
    if (device->classes & (INPUT_DEVICE_CLASS_KEYBOARD | INPUT_DEVICE_CLASS_JOYSTICK)) {
        // Load the keymap for the device.
        keyMapStatus = loadKeyMapLocked(device);
    }

    // Configure the keyboard, gamepad or virtual keyboard.
    if (device->classes & INPUT_DEVICE_CLASS_KEYBOARD) {
        // Register the keyboard as a built-in keyboard if it is eligible.
        if (!keyMapStatus
                && mBuiltInKeyboardId == NO_BUILT_IN_KEYBOARD
                && isEligibleBuiltInKeyboard(device->identifier,
                        device->configuration, &device->keyMap)) {
            mBuiltInKeyboardId = device->id;
        }

        // 'Q' key support = cheap test of whether this is an alpha-capable kbd
        if (hasKeycodeLocked(device, AKEYCODE_Q)) {
            device->classes |= INPUT_DEVICE_CLASS_ALPHAKEY;
        }

        // See if this device has a DPAD.
        if (hasKeycodeLocked(device, AKEYCODE_DPAD_UP) &&
                hasKeycodeLocked(device, AKEYCODE_DPAD_DOWN) &&
                hasKeycodeLocked(device, AKEYCODE_DPAD_LEFT) &&
                hasKeycodeLocked(device, AKEYCODE_DPAD_RIGHT) &&
                hasKeycodeLocked(device, AKEYCODE_DPAD_CENTER)) {
            device->classes |= INPUT_DEVICE_CLASS_DPAD;
        }

        // See if this device has a gamepad.
        for (size_t i = 0; i < sizeof(GAMEPAD_KEYCODES)/sizeof(GAMEPAD_KEYCODES[0]); i++) {
            if (hasKeycodeLocked(device, GAMEPAD_KEYCODES[i])) {
                device->classes |= INPUT_DEVICE_CLASS_GAMEPAD;
                break;
            }
        }

        // Disable kernel key repeat since we handle it ourselves
        unsigned int repeatRate[] = {0,0};
        if (ioctl(fd, EVIOCSREP, repeatRate)) {
            ALOGW("Unable to disable kernel key repeat for %s: %s", devicePath, strerror(errno));
        }
    }

    // If the device isn't recognized as something we handle, don't monitor it.
    if (device->classes == 0) {
        ALOGV("Dropping device: id=%d, path='%s', name='%s'",
                deviceId, devicePath, device->identifier.name.string());
        delete device;
        return -1;
    }

    // Determine whether the device has a mic.
    if (deviceHasMicLocked(device)) {
        device->classes |= INPUT_DEVICE_CLASS_MIC;
    }

    // Determine whether the device is external or internal.
    if (isExternalDeviceLocked(device)) {
        device->classes |= INPUT_DEVICE_CLASS_EXTERNAL;
    }

    if (device->classes & (INPUT_DEVICE_CLASS_JOYSTICK | INPUT_DEVICE_CLASS_DPAD)
            && device->classes & INPUT_DEVICE_CLASS_GAMEPAD) {
        device->controllerNumber = getNextControllerNumberLocked(device);
        // setLedForController(device);
    }
/*
    // Register with epoll.
    struct epoll_event eventItem;
    memset(&eventItem, 0, sizeof(eventItem));
    eventItem.events = EPOLLIN;
    eventItem.data.u32 = deviceId;
    if (epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, &eventItem)) {
        ALOGE("Could not add device fd to epoll instance.  errno=%d", errno);
        delete device;
        return -1;
    }

    String8 wakeMechanism("EPOLLWAKEUP");
    if (!mUsingEpollWakeup) {
#ifndef EVIOCSSUSPENDBLOCK
        // uapi headers don't include EVIOCSSUSPENDBLOCK, and future kernels
        // will use an epoll flag instead, so as long as we want to support
        // this feature, we need to be prepared to define the ioctl ourselves.
#define EVIOCSSUSPENDBLOCK _IOW('E', 0x91, int)
#endif
        if (ioctl(fd, EVIOCSSUSPENDBLOCK, 1)) {
            wakeMechanism = "<none>";
        } else {
            wakeMechanism = "EVIOCSSUSPENDBLOCK";
        }
    }
*/
    // Tell the kernel that we want to use the monotonic clock for reporting timestamps
    // associated with input events.  This is important because the input system
    // uses the timestamps extensively and assumes they were recorded using the monotonic
    // clock.
    //
    // In older kernel, before Linux 3.4, there was no way to tell the kernel which
    // clock to use to input event timestamps.  The standard kernel behavior was to
    // record a real time timestamp, which isn't what we want.  Android kernels therefore
    // contained a patch to the evdev_event() function in drivers/input/evdev.c to
    // replace the call to do_gettimeofday() with ktime_get_ts() to cause the monotonic
    // clock to be used instead of the real time clock.
    //
    // As of Linux 3.4, there is a new EVIOCSCLOCKID ioctl to set the desired clock.
    // Therefore, we no longer require the Android-specific kernel patch described above
    // as long as we make sure to set select the monotonic clock.  We do that here.
    int clockId = CLOCK_MONOTONIC;
    bool usingClockIoctl = !ioctl(fd, EVIOCSCLOCKID, &clockId);

    ALOGI("New device: id=%d, fd=%d, path='%s', name='%s', classes=0x%x, "
            "configuration='%s', keyLayout='%s', keyCharacterMap='%s', builtinKeyboard=%s, "
            "usingClockIoctl=%s",
         deviceId, fd, devicePath, device->identifier.name.string(),
         device->classes,
         device->configurationFile.string(),
         device->keyMap.keyLayoutFile.string(),
         device->keyMap.keyCharacterMapFile.string(),
         toString(mBuiltInKeyboardId == deviceId),
         toString(usingClockIoctl));

    addDeviceLocked(device);
    return 0;
}

static String8 generateDescriptor(InputDeviceIdentifier& identifier) {
    String8 rawDescriptor;
    rawDescriptor.appendFormat(":%04x:%04x:", identifier.vendor,
            identifier.product);
    // TODO add handling for USB devices to not uniqueify kbs that show up twice
    if (!identifier.uniqueId.isEmpty()) {
        rawDescriptor.append("uniqueId:");
        rawDescriptor.append(identifier.uniqueId);
    } else if (identifier.nonce != 0) {
        rawDescriptor.appendFormat("nonce:%04x", identifier.nonce);
    }

    if (identifier.vendor == 0 && identifier.product == 0) {
        // If we don't know the vendor and product id, then the device is probably
        // built-in so we need to rely on other information to uniquely identify
        // the input device.  Usually we try to avoid relying on the device name or
        // location but for built-in input device, they are unlikely to ever change.
        if (!identifier.name.isEmpty()) {
            rawDescriptor.append("name:");
            rawDescriptor.append(identifier.name);
        } else if (!identifier.location.isEmpty()) {
            rawDescriptor.append("location:");
            rawDescriptor.append(identifier.location);
        }
    }
    identifier.descriptor = sha1(rawDescriptor);
    // identifier.descriptor = rawDescriptor;
    return rawDescriptor;
}

void EventHub::assignDescriptorLocked(InputDeviceIdentifier& identifier) {
    // Compute a device descriptor that uniquely identifies the device.
    // The descriptor is assumed to be a stable identifier.  Its value should not
    // change between reboots, reconnections, firmware updates or new releases
    // of Android. In practice we sometimes get devices that cannot be uniquely
    // identified. In this case we enforce uniqueness between connected devices.
    // Ideally, we also want the descriptor to be short and relatively opaque.

    identifier.nonce = 0;
    String8 rawDescriptor = generateDescriptor(identifier);
    if (identifier.uniqueId.isEmpty()) {
        // If it didn't have a unique id check for conflicts and enforce
        // uniqueness if necessary.
        while(getDeviceByDescriptorLocked(identifier.descriptor) != NULL) {
            identifier.nonce++;
            rawDescriptor = generateDescriptor(identifier);
        }
    }
    ALOGV("Created descriptor: raw=%s, cooked=%s", rawDescriptor.string(),
            identifier.descriptor.string());
}

EventHub::Device* EventHub::getDeviceByDescriptorLocked(String8& descriptor) const {
    size_t size = mDevices.size();
    for (size_t i = 0; i < size; i++) {
        Device* device = mDevices.valueAt(i);
        if (descriptor.compare(device->identifier.descriptor) == 0) {
            return device;
        }
    }
    return NULL;
}

EventHub::Device* EventHub::getDeviceLocked(int32_t deviceId) const {
    if (deviceId == BUILT_IN_KEYBOARD_ID) {
        deviceId = mBuiltInKeyboardId;
    }
    ssize_t index = mDevices.indexOfKey(deviceId);
    return index >= 0 ? mDevices.valueAt(index) : NULL;
}

EventHub::Device* EventHub::getDeviceByPathLocked(const char* devicePath) const {
    for (size_t i = 0; i < mDevices.size(); i++) {
        Device* device = mDevices.valueAt(i);
        if (device->path == devicePath) {
            return device;
        }
    }
    return NULL;
}

void EventHub::addDeviceLocked(Device* device) {
    mDevices.add(device->id, device);
    // device->next = mOpeningDevices;
    // mOpeningDevices = device;
}

status_t EventHub::loadVirtualKeyMapLocked(Device* device) {
    // The virtual key map is supplied by the kernel as a system board property file.
    String8 path;
    path.append("/sys/board_properties/virtualkeys.");
    path.append(device->identifier.name);
    if (access(path.string(), R_OK)) {
        return NAME_NOT_FOUND;
    }
    return VirtualKeyMap::load(path, &device->virtualKeyMap);
}

status_t EventHub::loadKeyMapLocked(Device* device) {
    return device->keyMap.load(device->identifier, device->configuration);
}

bool EventHub::isExternalDeviceLocked(Device* device) {
    if (device->configuration) {
        bool value;
        if (device->configuration->tryGetProperty(String8("device.internal"), value)) {
            return !value;
        }
    }
    return device->identifier.bus == BUS_USB || device->identifier.bus == BUS_BLUETOOTH;
}

bool EventHub::deviceHasMicLocked(Device* device) {
    if (device->configuration) {
        bool value;
        if (device->configuration->tryGetProperty(String8("audio.mic"), value)) {
            return value;
        }
    }
    return false;
}

int32_t EventHub::getNextControllerNumberLocked(Device* device) {
    if (mControllerNumbers.isFull()) {
        ALOGI("Maximum number of controllers reached, assigning controller number 0 to device %s",
                device->identifier.name.string());
        return 0;
    }
    // Since the controller number 0 is reserved for non-controllers, translate all numbers up by
    // one
    return static_cast<int32_t>(mControllerNumbers.markFirstUnmarkedBit() + 1);
}

void EventHub::releaseControllerNumberLocked(Device* device) {
    int32_t num = device->controllerNumber;
    device->controllerNumber= 0;
    if (num == 0) {
        return;
    }
    mControllerNumbers.clearBit(static_cast<uint32_t>(num - 1));
}

void EventHub::scanDevicesLocked() {
    status_t res = scanDirLocked(DEVICE_PATH);
    if(res < 0) {
        ALOGE("scan dir failed for %s\n", DEVICE_PATH);
    }
/*
    if (mDevices.indexOfKey(VIRTUAL_KEYBOARD_ID) < 0) {
        createVirtualKeyboardLocked();
    }
*/
}

bool EventHub::hasKeycodeLocked(Device* device, int keycode) const {
    if (!device->keyMap.haveKeyLayout()) {
        return false;
    }

    Vector<int32_t> scanCodes;
    device->keyMap.keyLayoutMap->findScanCodesForKey(keycode, &scanCodes);
    const size_t N = scanCodes.size();
    for (size_t i=0; i<N && i<=KEY_MAX; i++) {
        int32_t sc = scanCodes.itemAt(i);
        if (sc >= 0 && sc <= KEY_MAX && test_bit(sc, device->keyBitmask)) {
            return true;
        }
    }

    return false;
}

status_t EventHub::closeDeviceByPathLocked(const char *devicePath) {
    Device* device = getDeviceByPathLocked(devicePath);
    if (device) {
        closeDeviceLocked(device);
        return 0;
    }
    ALOGV("Remove device: %s not found, device may already have been removed.", devicePath);
    return -1;
}

void EventHub::closeAllDevicesLocked() {
    while (mDevices.size() > 0) {
        closeDeviceLocked(mDevices.valueAt(mDevices.size() - 1));
    }
}

void EventHub::closeDeviceLocked(Device* device) {
    ALOGI("Removed device: path=%s name=%s id=%d fd=%d classes=0x%x\n",
         device->path.string(), device->identifier.name.string(), device->id,
         device->fd, device->classes);

    if (device->id == mBuiltInKeyboardId) {
        ALOGW("built-in keyboard device %s (id=%d) is closing! the apps will not like this",
                device->path.string(), mBuiltInKeyboardId);
        mBuiltInKeyboardId = NO_BUILT_IN_KEYBOARD;
    }
/*
    if (!device->isVirtual()) {
        if (epoll_ctl(mEpollFd, EPOLL_CTL_DEL, device->fd, NULL)) {
            ALOGW("Could not remove device fd from epoll instance.  errno=%d", errno);
        }
    }
*/
    releaseControllerNumberLocked(device);

    mDevices.removeItem(device->id);
    device->close();

    delete device;
}

status_t EventHub::readNotifyLocked() {
    int res;
    char devname[PATH_MAX];
    char *filename;
    char event_buf[512];
    int event_size;
    int event_pos = 0;
    struct inotify_event *event;

    ALOGV("EventHub::readNotify nfd: %d\n", mINotifyFd);
    res = read(mINotifyFd, event_buf, sizeof(event_buf));
    if(res < (int)sizeof(*event)) {
        if(errno == EINTR)
            return 0;
        ALOGW("could not get event, %s\n", strerror(errno));
        return -1;
    }
    //printf("got %d bytes of event information\n", res);

    strcpy(devname, DEVICE_PATH);
    filename = devname + strlen(devname);
    *filename++ = '/';

    while(res >= (int)sizeof(*event)) {
        event = (struct inotify_event *)(event_buf + event_pos);
        //printf("%d: %08x \"%s\"\n", event->wd, event->mask, event->len ? event->name : "");
        if(event->len) {
            strcpy(filename, event->name);
            if(event->mask & IN_CREATE) {
                openDeviceLocked(devname);
            } else {
                ALOGI("Removing device '%s' due to inotify event\n", devname);
                closeDeviceByPathLocked(devname);
            }
        }
        event_size = sizeof(*event) + event->len;
        res -= event_size;
        event_pos += event_size;
    }
    return 0;
}

status_t EventHub::scanDirLocked(const char *dirname)
{
    char devname[PATH_MAX];
    char *filename;
    DIR *dir;
    struct dirent *de;
    dir = opendir(dirname);
    if(dir == NULL)
        return -1;
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    while((de = readdir(dir))) {
        if(de->d_name[0] == '.' &&
           (de->d_name[1] == '\0' ||
            (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        openDeviceLocked(devname);
    }
    closedir(dir);
    return 0;
}

void EventHub::start()
{
    scanDevicesLocked();
}

void EventHub::loopOnce()
{
    size_t bufferSize = 256;
    struct input_event readBuffer[bufferSize];
    size_t capacity = bufferSize;

    mPendingEventIndex = 0;
    int timeoutMillis = 0;
    int pollResult = epoll_wait(mEpollFd, mPendingEventItems, EPOLL_MAX_EVENTS, timeoutMillis);

    // mLock.lock(); // reacquire lock after poll, must be after acquire_wake_lock
        if (pollResult == 0) {
            // Timed out.
            mPendingEventCount = 0;
            return;
        }

        if (pollResult < 0) {
            // An error occurred.
            mPendingEventCount = 0;

            // Sleep after errors to avoid locking up the system.
            // Hopefully the error is transient.
            if (errno != EINTR) {
                ALOGW("poll failed (errno=%d)\n", errno);
                usleep(100000);
            }
        } else {
            // Some events occurred.
            mPendingEventCount = size_t(pollResult);
        }

        // Grab the next input event.
        bool deviceChanged = false;
        while (mPendingEventIndex < mPendingEventCount) {
            const struct epoll_event& eventItem = mPendingEventItems[mPendingEventIndex++];
            if (eventItem.data.u32 == EPOLL_ID_INOTIFY) {
                if (eventItem.events & EPOLLIN) {
                    mPendingINotify = true;
                } else {
                    ALOGW("Received unexpected epoll event 0x%08x for INotify.", eventItem.events);
                }
                continue;
            }

            ssize_t deviceIndex = mDevices.indexOfKey(eventItem.data.u32);
            if (deviceIndex < 0) {
                ALOGW("Received unexpected epoll event 0x%08x for unknown device id %d.",
                        eventItem.events, eventItem.data.u32);
                continue;
            }

            Device* device = mDevices.valueAt(deviceIndex);
            if (eventItem.events & EPOLLIN) {
                int32_t readSize = read(device->fd, readBuffer,
                        sizeof(struct input_event) * capacity);
                if (readSize == 0 || (readSize < 0 && errno == ENODEV)) {
                    // Device was removed before INotify noticed.
                    ALOGW("could not get event, removed? (fd: %d size: %" PRId32
                            " bufferSize: %zu capacity: %zu errno: %d)\n",
                            device->fd, readSize, bufferSize, capacity, errno);
                    deviceChanged = true;
                    closeDeviceLocked(device);
                } else if (readSize < 0) {
                    if (errno != EAGAIN && errno != EINTR) {
                        ALOGW("could not get event (errno=%d)", errno);
                    }
                } else if ((readSize % sizeof(struct input_event)) != 0) {
                    ALOGE("could not get event (wrong size: %d)", readSize);
                } else {
                    int32_t deviceId = device->id == mBuiltInKeyboardId ? 0 : device->id;

                    size_t count = size_t(readSize) / sizeof(struct input_event);
                    for (size_t i = 0; i < count; i++) {
                        struct input_event& iev = readBuffer[i];
                        ALOGD("%s got: time=%d.%06d, type=%d, code=%d, value=%d",
                                device->path.string(),
                                (int) iev.time.tv_sec, (int) iev.time.tv_usec,
                                iev.type, iev.code, iev.value);
                    }

                }
            } else if (eventItem.events & EPOLLHUP) {
                ALOGI("Removing device %s due to epoll hang-up event.",
                        device->identifier.name.string());
                deviceChanged = true;
                closeDeviceLocked(device);
            } else {
                ALOGW("Received unexpected epoll event 0x%08x for device %s.",
                        eventItem.events, device->identifier.name.string());
            }
        }

        // readNotify() will modify the list of devices so this must be done after
        // processing all other events to ensure that we read all remaining events
        // before closing the devices.
        if (mPendingINotify && mPendingEventIndex >= mPendingEventCount) {
            mPendingINotify = false;
            readNotifyLocked();
            deviceChanged = true;
        }

}
/*
uint32_t sys2_send_input(uint32_t input_count, kosInput* inputs, int sizeof_input)
{
    Device* hitDevice = nullptr;
    size_t size = mDevices.size();
    for (size_t i = 0; i < size; i++) {
        Device* device = mDevices.valueAt(i);
        hitDevice = device;
        break;
    }
    if (hitDevice == nullptr) {
        return 0;
    }

    INPUT* inputs_dst = (INPUT*)malloc(sizeof(INPUT) * input_count);
    memset(inputs_dst, 0, sizeof(INPUT) * input_count);

    for (int n = 0; n < (int)input_count; n ++) {
        kosInput* src = inputs + n;
        INPUT* dst = inputs_dst + n;
        if (src->type == KOS_INPUT_MOUSE) {
            dst->type = INPUT_MOUSE;
            dst->mi.dx = src->u.mi.dx;
            dst->mi.dy = src->u.mi.dy;
            dst->mi.mouseData = src->u.mi.mouse_data;
            dst->mi.dwFlags = src->u.mi.flags;
            dst->mi.time = src->u.mi.time;
        }
    }
    write(hitDevice->fd,
    uint32_t ret = SendInput(input_count, inputs_dst, sizeof(INPUT));
    free(inputs_dst);
    return ret;
}
*/