# project-mark — macOS Port (Educational Reference)

This is a macOS port of [project-mark](https://github.com/vasuxd3v/project-mark), created purely as an
educational reference to compare Windows and macOS system programming concepts side-by-side.

---

## Purpose

The original project-mark was built for Windows and uses Win32-specific APIs throughout.
This port demonstrates the **macOS equivalents** of every Windows API used, keeping
the same project structure, class names, and logic — only the OS-level calls change.

---

## What Changed vs What Stayed the Same

### Identical (~60% of the codebase)
All of these are pure C++ and compile unchanged on any platform:
- **Luau VM** — `Module/Dependencies/Luau/`
- **Execution pipeline** — `Workspace/Execution/`
- **JSON, compression** — nlohmann, lz4, zstd
- **Task scheduler logic** — `TPService/Handler.cpp`, `Scheduler/TaskScheduler.cpp`
- **Yielder, Closures, Script modules** — pure Luau C API
- **ImGui core** — `imgui.cpp`, `imgui_draw.cpp`, etc.

### Modified (same file, different internals)

| File | Windows API | macOS API |
|------|-------------|-----------|
| `Main.mm` | `DllMain` + `DLL_PROCESS_ATTACH` | `__attribute__((constructor))` |
| `Engine/Engine.hpp` | `GetModuleHandleA(0)` | `_dyld_get_image_header(0)` |
| `Miscellaneous/Includes.hpp` | `<Windows.h>`, `<TlHelp32.h>`, `<Psapi.h>` | `<mach-o/dyld.h>`, `<mach/mach.h>`, `<signal.h>` |
| `Common/VEH/VEH.cpp` | `SetUnhandledExceptionFilter` + `CRITICAL_SECTION` | `sigaction` + `pthread_mutex_t` |
| `Common/Logger/Logger.cpp` | Named pipe (`\\.\pipe\...`) | UNIX domain socket (`AF_UNIX`) |
| `Communications/Communications.cpp` | Winsock2 (`WSAStartup`/`WSACleanup`) | BSD sockets (no init needed) |
| `Roblox/Bypassing/ThreadPool.hpp` | `CreateThreadpoolWork` | `dispatch_async` (GCD) |
| `Roblox/Bypassing/Hyperion.hpp` | `GetModuleInformation` + `MODULEINFO` | Walk `mach_header_64` load commands |
| `Roblox/Scheduler/Scheduler.cpp` | `VirtualQuery` + `MEMORY_BASIC_INFORMATION` | `vm_region_64` + Mach VM |
| `Workspace/AutoExecute/Autoexec.cpp` | `SHGetFolderPathA(CSIDL_LOCAL_APPDATA)` | `getenv("HOME")` + `~/Library/Application Support/` |
| `Workspace/Framework/Modules/FileLib/Filesystem.hpp` | `%LOCALAPPDATA%` | `~/Library/Application Support/project-mark/workspace/` |
| `Workspace/Framework/Modules/HTTP/HTTP.cpp` | `GetCurrentHwProfile` | IOKit `IOPlatformExpertDevice` UUID |
| `Workspace/Framework/Modules/Input/Input.mm` | `keybd_event`, `mouse_event`, `MessageBoxA` | `CGEventCreateKeyboardEvent`, `CGEventPost`, `NSAlert` |
| `Engine/Overlay/Overlay.mm` | DirectX 11 + `imgui_impl_dx11` | Metal + `imgui_impl_metal` |

### New Files

```
Module/Root/Darwin/
├── DyldUtils.hpp      — wraps _dyld_* API (replaces TlHelp32 / GetModuleHandle)
├── MemoryRegion.hpp   — wraps vm_region_64 (replaces VirtualQuery)
├── IOKitHWID.hpp      — C++ interface
└── IOKitHWID.mm       — IOPlatformExpertDevice UUID (replaces GetCurrentHwProfile)

CMakeLists.txt         — replaces project-mark.sln + binninsploit.vcxproj
```

---

## Key Concept: DLL → dylib

| | Windows | macOS |
|---|---|---|
| Library type | `.dll` (PE format) | `.dylib` (Mach-O format) |
| Entry point | `DllMain(HMODULE, DWORD, LPVOID)` | `__attribute__((constructor))` |
| Module query | `GetModuleHandleA(name)` | `_dyld_get_image_header(index)` |
| Memory query | `VirtualQuery(ptr, &mbi, ...)` | `vm_region_64(task, &addr, ...)` |
| Exception handler | `SetUnhandledExceptionFilter` | `sigaction(SIGSEGV, ...)` |
| Thread pool | `CreateThreadpoolWork` | `dispatch_async(global_queue)` |
| Mutex | `CRITICAL_SECTION` | `pthread_mutex_t` |
| Networking | Winsock2 (`WSAStartup` + BSD API) | BSD sockets (no init needed) |
| IPC | Named pipe (`\\.\pipe\name`) | UNIX domain socket (`/tmp/name`) |
| Input events | `keybd_event`, `mouse_event` | `CGEventCreateKeyboardEvent`, `CGEventPost` |
| GPU rendering | DirectX 11 | Metal |
| App data path | `%LOCALAPPDATA%\App\` | `~/Library/Application Support/App/` |

---

## Build

```bash
# Prerequisites
xcode-select --install
brew install cmake curl zstd

# Configure
cmake -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_OSX_ARCHITECTURES="arm64"  # or "x86_64" for Intel Mac

# Build
cmake --build build -j$(sysctl -n hw.logicalcpu)

# Output
file build/libproject-mark.dylib
# → Mach-O 64-bit dynamically linked shared library arm64
```

---

## Project Structure

```
project-mark-macOS/
├── CMakeLists.txt                          ← replaces project-mark.sln + .vcxproj
└── Module/
    ├── Dependencies/                       ← mostly identical to Windows
    │   ├── Luau/                           ← IDENTICAL
    │   ├── imgui/                          ← core identical, backends replaced
    │   ├── cpr/, nlohmann/, lz4/, zstd/   ← IDENTICAL
    └── Root/
        ├── Main.mm                         ← __attribute__((constructor))
        ├── Darwin/                         ← NEW — macOS system wrappers
        │   ├── DyldUtils.hpp
        │   ├── MemoryRegion.hpp
        │   └── IOKitHWID.hpp/.mm
        ├── Engine/
        │   ├── Engine.hpp                  ← _dyld_get_image_header
        │   └── Overlay/Overlay.mm          ← Metal backend
        ├── Miscellaneous/
        │   ├── Includes.hpp                ← macOS headers replace Windows headers
        │   ├── Common/VEH/VEH.cpp          ← sigaction replaces SEH
        │   ├── Common/Logger/Logger.cpp    ← UNIX socket replaces named pipe
        │   └── Communications/             ← BSD sockets (drop WSA calls)
        ├── Roblox/
        │   ├── Bypassing/ThreadPool.hpp    ← GCD replaces Windows thread pool
        │   ├── Bypassing/Hyperion.hpp      ← Mach-O walk replaces GetModuleInformation
        │   └── Scheduler/Scheduler.cpp     ← vm_region_64 replaces VirtualQuery
        └── Workspace/
            ├── AutoExecute/Autoexec.cpp    ← ~/Library path
            └── Framework/Modules/
                ├── FileLib/Filesystem.hpp  ← ~/Library path
                ├── HTTP/HTTP.cpp           ← IOKit HWID
                └── Input/Input.mm          ← CGEvent replaces keybd_event/mouse_event
```

---

## Credits

Original project by [vasuxd3v](https://github.com/vasuxd3v) — open-sourced for educational purposes.
macOS port created as a platform comparison reference.
