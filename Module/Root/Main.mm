// Cobalt - macOS Port
// Educational reference: demonstrates macOS equivalents of Windows DLL concepts
//
// Windows original: DllMain() with DLL_PROCESS_ATTACH + MessageBoxA
// macOS equivalent: __attribute__((constructor)) fires automatically when
//                   the .dylib is loaded via DYLD_INSERT_LIBRARIES or dlopen()
//
// Key concept: On Windows a DLL entry point is DllMain(HMODULE, DWORD, LPVOID).
//              On macOS there is no such entry point. Instead, any function
//              marked __attribute__((constructor)) is called by dyld (the dynamic
//              linker) as soon as the .dylib is mapped into the process — exactly
//              the same moment DLL_PROCESS_ATTACH fires on Windows.

#import <Foundation/Foundation.h>   // NSLog — replaces MessageBoxA for diagnostics
#include <thread>
#include "Includes.hpp"
#include <Bypassing/Hyperion.hpp>
#include "Common/VEH/VEH.hpp"
#include <Communications/Communications.hpp>

// ---------------------------------------------------------------------------
// Thread() — same logical sequence as the Windows version.
// Called from a detached std::thread so dyld is not blocked during init
// (same reason DllMain spawned a thread before doing real work).
// ---------------------------------------------------------------------------
void Thread()
{
    // 1. Register signal handlers (replaces SetUnhandledExceptionFilter)
    CVEH::Start();

    // 2. Start the UNIX-domain-socket logger (replaces named pipe logger)
    Logger::Init();

    // 3. Injection confirmation
    //    Windows used: MessageBoxA(nullptr, "Injected", "Cobalt | EntryPoint", MB_ICONINFORMATION)
    //    macOS: NSLog writes to Console.app / stderr — visible in Xcode console or `log stream`
    NSLog(@"[Cobalt] Injected into process");
    fprintf(stderr, "[Cobalt] Injected\n");

    // 4. Start the task-scheduler monitor (polls for DataModel changes)
    TPService->Initialize();

    // 5. Auto-execute stored scripts after a short delay
    std::thread([] {
        AutoExecution->Run();
    }).detach();
}

// ---------------------------------------------------------------------------
// __attribute__((constructor)) — THE key difference from Windows.
//
// Windows DLL entry point (original):
//   bool __stdcall DllMain(HMODULE Module, uintptr_t Reason, void*) {
//       if (Reason == DLL_PROCESS_ATTACH)
//           std::thread(Thread).detach();
//       return true;
//   }
//
// macOS .dylib entry point (this file):
//   __attribute__((constructor)) fires for DLL_PROCESS_ATTACH equivalent.
//   __attribute__((destructor))  fires for DLL_PROCESS_DETACH equivalent.
//
// There is no HMODULE, no Reason code, no return value — dyld handles all
// that internally. The constructor attribute is supported by both GCC and
// Clang (the only compiler on macOS), so it works universally.
// ---------------------------------------------------------------------------
__attribute__((constructor))
static void CobaltInit() {
    std::thread(Thread).detach();
}
