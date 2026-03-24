// Darwin/IOKitHWID.hpp
// macOS equivalent of Windows GetCurrentHwProfile / HW_PROFILE_INFO
//
// Windows approach:
//   HW_PROFILE_INFO hwProfileInfo;
//   GetCurrentHwProfile(&hwProfileInfo);
//   std::string hwid = hwProfileInfo.szHwProfileGuid;
//   // gives something like: {12345678-1234-1234-1234-123456789012}
//
// macOS approach:
//   Uses the IOKit framework to query the IOPlatformExpertDevice service.
//   This is a kernel service that exposes hardware info — it's the root of
//   the IOKit device tree, similar to the Windows hardware profile registry.
//
//   Key APIs:
//     IOServiceGetMatchingService()    — find the IOPlatformExpertDevice
//     IORegistryEntryCreateCFProperty()— read a property (like IOPlatformUUID)
//     IOObjectRelease()                — release the IOKit object reference
//
//   The "IOPlatformUUID" property is a machine-unique identifier that persists
//   across reboots (but may change on logic board replacement), exactly like
//   the Windows hardware profile GUID.
//
// Note: This file is a .hpp but the IMPLEMENTATION is in IOKitHWID.mm
// (Objective-C++) because IOKit requires linking -framework IOKit and the
// CoreFoundation CFString APIs are easiest to use from ObjC or ObjC++.
// The header exposes a clean C++ function so the rest of the codebase
// doesn't need to know about Objective-C.

#pragma once
#include <string>

namespace Darwin {

    // -----------------------------------------------------------------------
    // GetHardwareUUID
    // Windows equivalent: GetCurrentHwProfile(&hwProfileInfo)
    //                     then read hwProfileInfo.szHwProfileGuid
    //
    // Returns the IOPlatformUUID string, e.g.:
    //   "A1B2C3D4-E5F6-7890-ABCD-EF1234567890"
    //
    // Returns "Unknown" on failure (IOKit unavailable, SIP restriction, etc.)
    // -----------------------------------------------------------------------
    std::string GetHardwareUUID();

} // namespace Darwin
