// Darwin/DyldUtils.hpp
// macOS equivalent of Windows TlHelp32.h + GetModuleHandle / GetModuleInformation
//
// Windows approach:
//   GetModuleHandleA("name.dll")          — get base address of a loaded DLL
//   GetModuleHandleA(0)                   — get base of the main executable
//   EnumProcessModules() / TlHelp32       — iterate all loaded modules
//
// macOS approach:
//   _dyld_get_image_header(index)         — Mach-O header pointer (= base address)
//   _dyld_get_image_name(index)           — full path of the image
//   _dyld_image_count()                   — number of loaded images
//   _dyld_get_image_vmaddr_slide(index)   — ASLR slide for this image
//
// The dyld API lives in <mach-o/dyld.h> and is always available — no import
// library needed (unlike Psapi.lib on Windows).
//
// Concepts demonstrated:
//   - Mach-O image enumeration (vs DLL enumeration with TlHelp32)
//   - ASLR slides: on macOS every image has a "slide" (the difference between
//     the preferred and actual load address). The base address you get from
//     _dyld_get_image_header is ALREADY slid — no REBASE macro needed if you
//     use it directly. The slide is useful when you have a preferred-address
//     offset from a disassembler and need to rebase it at runtime.

#pragma once
#include <cstdint>
#include <string>
#include <mach-o/dyld.h>   // _dyld_* functions

namespace Darwin {

    // -----------------------------------------------------------------------
    // GetImageCount
    // Windows equivalent: EnumProcessModules() — returns count of loaded DLLs
    // macOS: _dyld_image_count() returns the number of Mach-O images mapped
    //        into this process (executable + all .dylibs + system frameworks)
    // -----------------------------------------------------------------------
    inline uint32_t GetImageCount() {
        return _dyld_image_count();
    }

    // -----------------------------------------------------------------------
    // GetImageBase
    // Windows equivalent: GetModuleHandleA(nullptr) for index 0 (main exe),
    //                     or GetModuleHandleA("name.dll") for a named DLL.
    // macOS: _dyld_get_image_header(index) returns a pointer to the
    //        mach_header (32-bit) or mach_header_64 (64-bit) struct at the
    //        very start of the image — this IS the base address.
    //
    // Note: index 0 is always the main executable, just like GetModuleHandleA(0).
    // -----------------------------------------------------------------------
    inline uintptr_t GetImageBase(uint32_t index) {
        const struct mach_header* hdr = _dyld_get_image_header(index);
        return reinterpret_cast<uintptr_t>(hdr);
    }

    // -----------------------------------------------------------------------
    // GetImageName
    // Windows equivalent: GetModuleFileNameA(hModule, buf, size)
    // macOS: _dyld_get_image_name(index) returns the full filesystem path
    //        of the image (e.g. "/usr/lib/libSystem.B.dylib")
    // -----------------------------------------------------------------------
    inline const char* GetImageName(uint32_t index) {
        return _dyld_get_image_name(index);
    }

    // -----------------------------------------------------------------------
    // GetImageSlide
    // Windows equivalent: N/A (Windows ASLR uses a different model)
    // macOS: Every image has a "slide" — the offset added to all preferred
    //        virtual addresses due to ASLR. If your disassembler shows a
    //        symbol at 0x100001234 (preferred), the actual runtime address
    //        is 0x100001234 + slide.
    //
    // This is the macOS version of the REBASE() macro concept.
    // -----------------------------------------------------------------------
    inline intptr_t GetImageSlide(uint32_t index) {
        return _dyld_get_image_vmaddr_slide(index);
    }

    // -----------------------------------------------------------------------
    // FindImageByName
    // Windows equivalent: GetModuleHandleA("RobloxPlayerBeta.dll")
    // macOS: Iterates all loaded images and does a substring match on the
    //        path. Returns the base address (mach_header pointer) or 0.
    //
    // Example usage:
    //   uintptr_t base = Darwin::FindImageByName("RobloxPlayer");
    //   // base is the equivalent of GetModuleHandle("RobloxPlayerBeta.dll")
    // -----------------------------------------------------------------------
    inline uintptr_t FindImageByName(const char* partialName) {
        uint32_t count = _dyld_image_count();
        for (uint32_t i = 0; i < count; i++) {
            const char* name = _dyld_get_image_name(i);
            if (name && strstr(name, partialName)) {
                return reinterpret_cast<uintptr_t>(_dyld_get_image_header(i));
            }
        }
        return 0;
    }

    // -----------------------------------------------------------------------
    // GetMainExecutableBase
    // Windows equivalent: GetModuleHandleA(nullptr) or GetModuleHandleA(0)
    // macOS: Index 0 in the dyld image list is always the main executable.
    // -----------------------------------------------------------------------
    inline uintptr_t GetMainExecutableBase() {
        return GetImageBase(0);
    }

} // namespace Darwin
