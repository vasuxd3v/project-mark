// Darwin/IOKitHWID.mm  (Objective-C++ — must be .mm for IOKit/CoreFoundation)
// macOS equivalent of Windows GetCurrentHwProfile
//
// This file is .mm (Objective-C++) for two reasons:
//   1. IOKit uses CoreFoundation types (CFStringRef, CFAllocatorRef) which
//      are easiest to work with in ObjC++ context.
//   2. The CoreFoundation toll-free bridging (CFString <-> NSString) is
//      available, making string conversion trivial with @() syntax.
//
// On Windows the equivalent is a single Win32 API call:
//   HW_PROFILE_INFO info;
//   GetCurrentHwProfile(&info);
//   // info.szHwProfileGuid is the GUID
//
// On macOS, we traverse the IOKit device tree:
//   IOServiceGetMatchingService → finds IOPlatformExpertDevice
//   IORegistryEntryCreateCFProperty → reads the "IOPlatformUUID" key
//
// The IOKit device tree is analogous to the Windows device tree in Device
// Manager, but accessed programmatically via a kernel IPC mechanism.

#import <Foundation/Foundation.h>
#import <IOKit/IOKitLib.h>          // IOServiceGetMatchingService, IOObjectRelease
                                    // Requires: -framework IOKit
#include "IOKitHWID.hpp"

namespace Darwin {

std::string GetHardwareUUID() {
    // -----------------------------------------------------------------------
    // Step 1: Get the IOPlatformExpertDevice service
    //
    // IOServiceGetMatchingService searches the IOKit registry (device tree)
    // for a service whose class name matches "IOPlatformExpertDevice".
    // This is always present on Apple hardware — it's the root of the tree.
    //
    // kIOMainPortDefault (macOS 12+) or kIOMasterPortDefault (older) is the
    // Mach port that connects us to the IOKit registry. It's like the root
    // handle for all device tree queries.
    //
    // Windows analogy: RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\...", ...)
    // -----------------------------------------------------------------------
    io_service_t platformExpert = IOServiceGetMatchingService(
        kIOMainPortDefault,
        IOServiceMatching("IOPlatformExpertDevice")
    );

    if (!platformExpert) {
        return "Unknown";
    }

    // -----------------------------------------------------------------------
    // Step 2: Read the "IOPlatformUUID" property
    //
    // IORegistryEntryCreateCFProperty reads a named property from an IOKit
    // registry entry and returns it as a CFTypeRef (here, a CFStringRef).
    //
    // kIOPlatformUUIDKey is the constant string "IOPlatformUUID".
    //
    // Windows analogy: RegGetValueA(key, nullptr, "HardwareProfileGuid", ...)
    // -----------------------------------------------------------------------
    CFStringRef uuidRef = (CFStringRef)IORegistryEntryCreateCFProperty(
        platformExpert,
        CFSTR(kIOPlatformUUIDKey),   // "IOPlatformUUID"
        kCFAllocatorDefault,
        0                            // options (0 = default)
    );

    // -----------------------------------------------------------------------
    // Step 3: Release the IOKit object
    //
    // Every io_service_t obtained from IOServiceGetMatchingService must be
    // released with IOObjectRelease — analogous to CloseHandle on Windows.
    // -----------------------------------------------------------------------
    IOObjectRelease(platformExpert);

    if (!uuidRef) {
        return "Unknown";
    }

    // -----------------------------------------------------------------------
    // Step 4: Convert CFStringRef → std::string
    //
    // On Windows the GUID comes back as a wchar_t[] in HW_PROFILE_INFO.
    // On macOS we have a CFStringRef. CFStringGetCString converts it to
    // a C string using the specified encoding (UTF-8 here).
    // -----------------------------------------------------------------------
    char buffer[64] = {};
    CFStringGetCString(uuidRef, buffer, sizeof(buffer), kCFStringEncodingUTF8);
    CFRelease(uuidRef);

    return std::string(buffer);
}

} // namespace Darwin
