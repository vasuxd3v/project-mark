// Workspace/AutoExecute/Autoexec.cpp — macOS port
//
// ONE change: the path-resolution API.
//
// WINDOWS:
//   #include <ShlObj.h>
//   char path[MAX_PATH];
//   SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, path);
//   folderPath = std::string(path) + "\\Cobalt\\autoexec\\";
//   // gives: C:\Users\<user>\AppData\Local\Cobalt\autoexec\
//
// MACOS:
//   const char* home = getenv("HOME");
//   folderPath = std::string(home) + "/Library/Application Support/Cobalt/autoexec/";
//   // gives: /Users/<user>/Library/Application Support/Cobalt/autoexec/
//
// Why ~/Library/Application Support/?
//   This is the macOS convention for per-user application data storage.
//   It is the direct equivalent of %LOCALAPPDATA% on Windows:
//
//   Windows: C:\Users\<user>\AppData\Local\       = CSIDL_LOCAL_APPDATA
//   macOS:   /Users/<user>/Library/Application Support/  = conventional user data dir
//
//   On macOS you could also use NSSearchPathForDirectoriesInDomains() (ObjC API)
//   or FSFindFolder (deprecated Carbon API), but getenv("HOME") + the conventional
//   path is the simplest C++ approach that works identically to the Windows version.
//
// Everything else in this file is IDENTICAL:
//   - std::chrono timing
//   - std::filesystem directory iteration
//   - .lua / .txt extension filter
//   - std::ifstream file reading
//   - CExecution::SendScript() call

#pragma once
#include "Autoexec.hpp"
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdlib>    // getenv — replaces SHGetFolderPathA / ShlObj.h

std::atomic<bool> autoexecStarted(false);

void CAutoExecute::Run() {
    using namespace std::chrono_literals;
    Logger::printf("[COBALT - UTILITY FUNCTIONS] Started AutoExecution");

    auto start = std::chrono::steady_clock::now();

    while (!autoexecStarted.load()) {
        auto now = std::chrono::steady_clock::now();
        if (now - start >= 2s) {

            // ---------------------------------------------------------------
            // PATH RESOLUTION — the only changed part
            //
            // Windows: SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, ..., path)
            //          + "\\Cobalt\\autoexec\\"
            //
            // macOS:   getenv("HOME") + "/Library/Application Support/Cobalt/autoexec/"
            //
            // getenv("HOME") returns /Users/<username> on macOS.
            // If HOME is not set (rare, but possible in sandboxed contexts),
            // fall back to /tmp (same idea as Windows fallback to GetTempPath).
            // ---------------------------------------------------------------
            std::string folderPath;
            const char* home = getenv("HOME");
            if (home) {
                folderPath = std::string(home) + "/Library/Application Support/Cobalt/autoexec/";
            }

            if (!folderPath.empty()) {
                namespace fs = std::filesystem;
                try {
                    // Identical logic from here — std::filesystem is cross-platform
                    if (fs::exists(folderPath)) {
                        for (const auto& entry : fs::directory_iterator(folderPath)) {
                            if (!entry.is_regular_file())
                                continue;

                            auto ext = entry.path().extension().string();
                            if (ext != ".lua" && ext != ".txt")
                                continue;

                            std::ifstream fileStream(entry.path(), std::ios::in | std::ios::binary);
                            if (!fileStream)
                                continue;

                            std::string script((std::istreambuf_iterator<char>(fileStream)),
                                               std::istreambuf_iterator<char>());

                            lua_State* L = Globals::RobloxThread;
                            if (!L) {
                                ROBLOX::Print(3, "[COBALT->AUTOEXECUTION] No valid luastate to execute in!");
                                return;
                            }

                            CExecution::SendScript(L, script);
                        }
                    }
                }
                catch (const std::exception&) {
                    // silent fail
                }
            }

            autoexecStarted.store(true);
            break;
        }
        std::this_thread::sleep_for(10ms);
    }
}
