// Miscellaneous/Includes.hpp — macOS port
// TYPE SHIMS — must be absolutely first
#include <cstdint>   // uint32_t, uint8_t, int64_t, int32_t — needed before the aliases below
#define _XOPEN_SOURCE 700  // required for ucontext_t on macOS
#ifndef DWORD
    using DWORD = uint32_t;
#endif
// Note: BOOL is intentionally not defined here.
//       ObjC/ObjC++ headers define BOOL as bool — redefining it causes errors.
//       The original Windows code uses BOOL in a few places; those are
//       handled by the fact that in .cpp context they're function return values
//       where plain int works fine.
#ifndef BYTE
    using BYTE  = uint8_t;
#endif
#ifndef __forceinline
    #define __forceinline __attribute__((always_inline)) inline
#endif
#ifndef __int64
    #define __int64 int64_t
#endif
#ifndef __int32
    #define __int32 int32_t
#endif
// END TYPE SHIMS
// This is the master include file. It's the highest-changed file in the port.
//
// REMOVED (Windows-only):
//   #include <Windows.h>       — core Win32 API
//   #include <TlHelp32.h>      — process/module/thread enumeration (Tool Help)
//   #include <Psapi.h>         — process status API (GetModuleInformation etc.)
//   #define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1  (still needed if using CryptoPP)
//
// ADDED (macOS/POSIX):
//   #include <unistd.h>            — getpid(), close(), write(), read()
//   #include <sys/types.h>         — POSIX type definitions
//   #include <dlfcn.h>             — dlopen(), dlsym(), dlclose()
//   #include <mach-o/dyld.h>       — _dyld_* image enumeration
//   #include <mach/mach.h>         — Mach kernel APIs (task_self, vm_region)
//   #include <mach/vm_map.h>       — vm_region_64, vm_allocate
//   #include <mach/vm_prot.h>      — VM_PROT_READ/WRITE/EXECUTE
//
// UNCHANGED: all standard C++ headers, Luau VM headers, nlohmann, zstd, etc.
//
// CRYPTO NOTE:
//   The original uses CryptoPP. CryptoPP is cross-platform C++ and compiles
//   on macOS without changes. Alternatively, Apple's CommonCrypto is available:
//     #include <CommonCrypto/CommonCrypto.h>
//   and provides MD5, SHA1, SHA256, AES equivalents natively.

#pragma once

// ---------------------------------------------------------------------------
// POSIX / Darwin system headers (replace Windows.h, TlHelp32.h, Psapi.h)
// ---------------------------------------------------------------------------
#include <unistd.h>            // getpid, close, write, sleep
#include <sys/types.h>         // pid_t, size_t, etc.
#include <dlfcn.h>             // dlopen, dlsym, dlclose — replaces LoadLibrary/GetProcAddress
#include <mach-o/dyld.h>       // _dyld_image_count, _dyld_get_image_header — replaces TlHelp32
#include <mach/mach.h>         // mach_task_self, kern_return_t — replaces GetCurrentProcess
#include <mach/vm_map.h>       // vm_region_64 — replaces VirtualQuery
#include <mach/vm_prot.h>      // VM_PROT_READ/WRITE/EXECUTE — replaces PAGE_* constants
#include <pthread.h>           // pthread_mutex_t — replaces CRITICAL_SECTION
#include <signal.h>            // sigaction, SIGSEGV — replaces SetUnhandledExceptionFilter
#include <sys/socket.h>        // socket, bind, listen — replaces winsock2.h (same BSD API)
#include <netinet/in.h>        // sockaddr_in, INADDR_ANY
#include <arpa/inet.h>         // ntohl, htons
#include <sys/un.h>            // sockaddr_un — for UNIX domain sockets (replaces named pipes)

// ---------------------------------------------------------------------------
// Standard C++ (IDENTICAL to original — all platform-agnostic)
// ---------------------------------------------------------------------------
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <map>
#include <cstdlib>
#include <stdexcept>
#include <ctime>
#include <cctype>
#include <queue>
#include <random>
#include <sstream>
#include <functional>
#include <memory>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <atomic>
#include <chrono>

// ---------------------------------------------------------------------------
// Luau VM headers (IDENTICAL — Luau is cross-platform C++)
// ---------------------------------------------------------------------------
#include <lua.h>
#include <lualib.h>
#include <lstate.h>
#include "VM/src/lcommon.h"
#include "VM/src/lstring.h"
#include "VM/src/lfunc.h"
#include "VM/src/lmem.h"
#include "VM/src/lgc.h"
#include "VM/src/ltable.h"
#include "VM/src/lobject.h"
#include "VM/src/lapi.h"
#include "Reflections.hpp"
#include "VM/src/ldo.h"

// ---------------------------------------------------------------------------
// Project headers
// ---------------------------------------------------------------------------
#include <Bytecode/Bytecode.hpp>
#include <Engine.hpp>

// ---------------------------------------------------------------------------
// Crypto — CryptoPP is cross-platform C++, compiles on macOS with no changes.
// If you prefer native macOS crypto, use #include <CommonCrypto/CommonCrypto.h>
// ---------------------------------------------------------------------------
#include <cryptopp/md5.h>
#include <cryptopp/sha3.h>
#include <cryptopp/sha.h>
#include <cryptopp/base64.h>
#include <cryptopp/hex.h>
#include <cryptopp/rdrand.h>
#include <cryptopp/modes.h>
#include <cryptopp/aes.h>

// ---------------------------------------------------------------------------
// Third-party (IDENTICAL — all cross-platform)
// ---------------------------------------------------------------------------
#include <nlohmann/json.hpp>
#include "zstd/include/zstd/zstd.h"
#include "Compiler/include/Luau/Compiler.h"
#include "Compiler/include/luacode.h"
#include "Common/include/Luau/BytecodeUtils.h"
#include "Compiler/include/Luau/BytecodeBuilder.h"

// ---------------------------------------------------------------------------
// Internal module headers (IDENTICAL)
// ---------------------------------------------------------------------------
#include <Execution/Execution.hpp>
#include <Scheduler/Scheduling.hpp>
#include <TPService/Handler.hpp>
#include <Overlay/Overlay.hpp>
#include <Rendering/imgui-notify.h>
#include <Common/Logger/Logger.hpp>
#include <Yieldworker/Yielder.hpp>
#include <AutoExecute/Autoexec.hpp>

// (type shims moved to top of file)
