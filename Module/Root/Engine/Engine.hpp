// Engine/Engine.hpp — macOS port
// Original used Windows-specific GetModuleHandleA() to get the base address.
// macOS uses the dyld (dynamic linker) API instead.
//
// WINDOWS (original):
//   #include <Windows.h>
//   static uintptr_t Roblox_BASE = (uintptr_t)GetModuleHandleA(0);
//   inline const uintptr_t HYPERION = (uintptr_t)GetModuleHandle("RobloxPlayerBeta.dll");
//
// MACOS (this file):
//   #include <mach-o/dyld.h>
//   static uintptr_t Roblox_BASE = (uintptr_t)_dyld_get_image_header(0);
//   inline const uintptr_t HYPERION = Darwin::FindImageByName("RobloxPlayer");
//
// Why index 0 works the same way:
//   On Windows, GetModuleHandleA(0) returns the base of the current .exe.
//   On macOS, _dyld_get_image_header(0) returns the mach_header* of the main
//   executable — index 0 in the dyld image list is always the main image.
//   Both give you the same thing: the load address of the process's own binary.
//
// ASLR / REBASE:
//   Both Windows and macOS randomize load addresses (ASLR).
//   The REBASE(x) macro adds the runtime base to a compile-time offset,
//   exactly the same concept on both platforms.
//   On macOS you can also use _dyld_get_image_vmaddr_slide(0) if you have
//   preferred-address offsets from a disassembler.

#pragma once
#include <iostream>
#include <vector>
#include <mach-o/dyld.h>       // _dyld_get_image_header, _dyld_image_count
#include <cstring>             // strstr
#include "LuaVM.hpp"
#include "Encryptions.hpp"
#include "Darwin/DyldUtils.hpp"

// ---------------------------------------------------------------------------
// Base address of the main executable
// Windows: GetModuleHandleA(0)  →  HMODULE of the .exe
// macOS:   _dyld_get_image_header(0)  →  mach_header* (== load address)
// ---------------------------------------------------------------------------
static uintptr_t Roblox_BASE = Darwin::GetMainExecutableBase();

// REBASE: same macro, same concept — add the runtime base to a static offset
#define REBASE(x) ((x) + Roblox_BASE)

// ---------------------------------------------------------------------------
// Base address of Hyperion (the anti-cheat module)
// Windows: GetModuleHandle("RobloxPlayerBeta.dll")
// macOS:   scan all loaded dylibs for one whose path contains "RobloxPlayer"
// ---------------------------------------------------------------------------
inline const uintptr_t HYPERION = Darwin::FindImageByName("RobloxPlayer");
#define HYP_REBASE(x) ((x) + HYPERION)

// ---------------------------------------------------------------------------
// Offsets — these are process-specific and would differ for the macOS binary
// (a macOS build of Roblox would have different addresses than Windows).
// Included here to show the same structural pattern; actual values would need
// to be determined via reverse engineering of the macOS Roblox binary.
// ---------------------------------------------------------------------------
namespace Offsets {

    const uintptr_t Print            = REBASE(0x17E0070);
    const uintptr_t PushInstance     = REBASE(0x106FE10);

    /* Luau addresses */
    const uintptr_t LuaH_DummyNode  = REBASE(0x577F2A8);
    const uintptr_t Luau_Execute     = REBASE(0x382DE34);
    const uintptr_t LuaD_throw       = REBASE(0x3826950);
    const uintptr_t LuaO_NilObject   = REBASE(0x577F8B8);

    const uintptr_t Bitmap           = HYP_REBASE(0x1C1280);
    const uintptr_t GetModuleFromVMStateMap = REBASE(0x1468EA0);

    namespace Lua {
        const uintptr_t OpcodeLookupTable = REBASE(0x5c4ee20);
    }

    namespace External {

        namespace Bytecode {
            const uintptr_t LocalScript  = 0x1A8;
            const uintptr_t ModuleScript = 0x150;
        }

        namespace UserData {
            const uintptr_t Capabilities = 0x68;
            const uintptr_t Identity     = 0x48;
        }

        namespace DataModel {
            const uintptr_t GameLoaded   = 0x5F8;
            constexpr uintptr_t Children = 0x70;
        }

        namespace TaskScheduler {
            constexpr uintptr_t FakeDMtoDM   = 0x1C0;
            const uintptr_t FakeDMPointer     = REBASE(0x7FF0818);
            const uintptr_t ScriptContext     = 0x3F0;
            constexpr uintptr_t PlaceId       = 0x190;
            constexpr uintptr_t JobEnd        = 0x1D8;
            constexpr uintptr_t JobId         = 0x138;
            constexpr uintptr_t JobName       = 0x18;
            constexpr uintptr_t JobStart      = 0x1D0;
        }
    }
}

// ---------------------------------------------------------------------------
// Function pointer declarations
// __fastcall on Windows is a Windows-specific calling convention.
// On macOS (System V AMD64 ABI / ARM64 ABI), there is only one calling
// convention — all functions use the same ABI. No __fastcall/__stdcall needed.
// ---------------------------------------------------------------------------
namespace ROBLOX {
    // Windows: void(__fastcall*)(int, const char*, ...)
    // macOS:   void(*)(int, const char*, ...)  — same signature, no convention keyword
    inline auto Print        = (uintptr_t(*)(int, const char*, ...))Offsets::Print;
    inline auto Luau_Execute = (void(*)(lua_State*))Offsets::Luau_Execute;
    inline auto LuaD_Throw   = (void(*)(lua_State*, int))Offsets::LuaD_throw;
    inline auto PushInstance = (uintptr_t*(*)(lua_State*, uintptr_t))Offsets::PushInstance;
}
