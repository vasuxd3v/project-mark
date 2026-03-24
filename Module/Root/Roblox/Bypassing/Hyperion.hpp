// Roblox/Bypassing/Hyperion.hpp — macOS port
//
// WINDOWS ORIGINAL:
//   void Bitmap(HMODULE module) {
//       MODULEINFO mi;
//       GetModuleInformation(GetCurrentProcess(), module, &mi, sizeof(mi));
//       auto base = (uintptr_t)module;
//       auto end  = base + mi.SizeOfImage;
//       // iterate pages and set bitmap bits...
//   }
//
//   GetModuleInformation (from Psapi.h) gives us:
//     mi.lpBaseOfDll  — base address (same as the module handle)
//     mi.SizeOfImage  — total size of the image in memory
//     mi.EntryPoint   — address of the entry point
//
// MACOS EQUIVALENT:
//   Parse the Mach-O load commands to find the __TEXT segment's vmsize.
//   This gives us the equivalent of SizeOfImage.
//
// MACH-O vs PE structure:
//   PE (Windows .exe/.dll):
//     DOS header → PE header → Optional header (SizeOfImage here) → Section table
//
//   Mach-O (macOS .dylib/.app):
//     mach_header_64 → load commands (LC_SEGMENT_64 for each segment)
//     The __TEXT segment contains code (executable pages), analogous to .text + headers
//     The __DATA segment contains mutable data, analogous to .data + .bss
//
//   On Windows, SizeOfImage = total virtual size of all sections.
//   On macOS,  there's no single "SizeOfImage" field — you sum the vmsize
//   of all LC_SEGMENT_64 load commands, or use just __TEXT for code pages.
//
// KEY API CHANGE:
//   Windows: GetModuleInformation() from <Psapi.h> (needs Psapi.lib)
//   macOS:   Walk load_command chain from mach_header_64 (no library needed)
//            mach_header_64 and load_command types from <mach-o/loader.h>

#pragma once
#include "Includes.hpp"
#include <mach-o/loader.h>   // mach_header_64, load_command, segment_command_64
                             // MH_MAGIC_64, LC_SEGMENT_64, LOAD_COMMAND_TYPES

struct Bypasser
{
    // -----------------------------------------------------------------------
    // Bitmap(imageBase)
    //
    // Windows signature: void Bitmap(HMODULE module)
    //   HMODULE is a pointer to the PE header, same as the base address.
    //
    // macOS signature: void Bitmap(uintptr_t imageBase)
    //   We pass the base address directly (from _dyld_get_image_header).
    //
    // The bitmap manipulation logic is IDENTICAL — only the way we get
    // the image size changes.
    // -----------------------------------------------------------------------
    void Bitmap(uintptr_t imageBase)
    {
        // -----------------------------------------------------------------------
        // Step 1: Get the Mach-O header
        //
        // Windows: HMODULE already IS the PE header pointer.
        // macOS:   imageBase IS the mach_header_64 pointer.
        //
        // mach_header_64.magic should be MH_MAGIC_64 (0xFEEDFACF).
        // Checking this prevents crashes on invalid base addresses.
        // -----------------------------------------------------------------------
        auto* header = reinterpret_cast<const mach_header_64*>(imageBase);
        if (!header || header->magic != MH_MAGIC_64) return;

        // -----------------------------------------------------------------------
        // Step 2: Walk load commands to find __TEXT segment size
        //
        // Windows: GetModuleInformation(GetCurrentProcess(), module, &mi, sizeof(mi))
        //          → mi.SizeOfImage gives the total image size
        //
        // macOS: The mach_header_64 is followed immediately by `ncmds` load
        //        commands. Each load_command has a `cmd` (type) and `cmdsize`.
        //        We iterate them looking for LC_SEGMENT_64 named "__TEXT".
        //
        // __TEXT segment properties:
        //   .vmaddr  = preferred load address (before ASLR slide)
        //   .vmsize  = size of the segment in virtual memory
        //   .fileoff = offset within the file
        //   .nsects  = number of sections (e.g., __text, __stubs, __const)
        //
        // For the bitmap, we only need vmsize (equivalent of SizeOfImage).
        // -----------------------------------------------------------------------
        auto* cmd = reinterpret_cast<const load_command*>(header + 1);
        uintptr_t segBase = 0;
        uintptr_t segEnd  = 0;

        for (uint32_t i = 0; i < header->ncmds; i++) {
            if (cmd->cmd == LC_SEGMENT_64) {
                auto* seg = reinterpret_cast<const segment_command_64*>(cmd);
                // Use __TEXT for executable pages (equivalent of SizeOfImage logic)
                if (strncmp(seg->segname, "__TEXT", 16) == 0) {
                    segBase = imageBase;                          // ASLR-slid base
                    segEnd  = imageBase + seg->vmsize;            // base + size
                    break;
                }
            }
            // Advance to next load command using cmdsize
            cmd = reinterpret_cast<const load_command*>(
                reinterpret_cast<const uint8_t*>(cmd) + cmd->cmdsize);
        }

        if (segBase == 0 || segEnd == 0) return;

        // -----------------------------------------------------------------------
        // Step 3: Bitmap manipulation — IDENTICAL to Windows version
        //
        // Windows: auto bmp = *(uintptr_t*)Offsets::Bitmap;
        // macOS:   same — Offsets::Bitmap is a runtime address in the target binary
        //
        // The page-by-page iteration (0x1000 = 4KB pages) is identical on both
        // platforms. x86_64 and ARM64 both use 4KB pages by default.
        // -----------------------------------------------------------------------
        auto bmp = *(uintptr_t*)Offsets::Bitmap;

        for (auto p = segBase; p < segEnd; p += 0x1000)
            *(uint8_t*)(bmp + (p >> 0x13)) |= 1 << ((p >> 0x10) & 7);
    }
};

inline auto Patching = std::make_unique<Bypasser>();
