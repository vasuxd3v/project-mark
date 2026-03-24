// Workspace/Framework/Modules/FileLib/Filesystem.hpp — macOS port
//
// TWO changes:
//   1. Path resolution: getenv("LOCALAPPDATA") → getenv("HOME") + conventional path
//   2. Path separator:  backslash "\\" → forward slash "/"
//
// WINDOWS:
//   inline std::filesystem::path a = getenv("LOCALAPPDATA");
//   inline std::filesystem::path b = a / "Cobalt";
//   inline std::filesystem::path c = b / "workspace";
//   return c.string() + "\\";
//
// MACOS:
//   inline std::filesystem::path a = std::string(getenv("HOME") ?: "/tmp");
//   inline std::filesystem::path c = a / "Library" / "Application Support" / "Cobalt" / "workspace";
//   return c.string() + "/";
//
// Note: std::filesystem::path::operator/ works correctly on both platforms,
//       producing "\" on Windows and "/" on macOS. Using it to build paths
//       is more portable than string concatenation with a hardcoded separator.
//
// The rest of the file (_SplitString, CFilesys) is IDENTICAL.

#pragma once
#include "Includes.hpp"
#include <cstdlib>    // getenv

// ---------------------------------------------------------------------------
// Workspace directory path
// Windows: %LOCALAPPDATA%\Cobalt\workspace\   (C:\Users\X\AppData\Local\...)
// macOS:   ~/Library/Application Support/Cobalt/workspace/
// ---------------------------------------------------------------------------
inline std::filesystem::path _cobalt_home = []() -> std::filesystem::path {
    const char* home = getenv("HOME");
    return home ? std::filesystem::path(home) : std::filesystem::path("/tmp");
}();

inline std::filesystem::path _cobalt_workspace =
    _cobalt_home / "Library" / "Application Support" / "Cobalt" / "workspace";

inline std::string WorkspaceDirectory() {
    if (!std::filesystem::exists(_cobalt_workspace)) {
        std::filesystem::create_directories(_cobalt_workspace);
    }
    // Return with trailing slash — "/" on macOS (was "\\" on Windows)
    return _cobalt_workspace.string() + "/";
}

// IDENTICAL to Windows version — pure C++ string manipulation
__forceinline void _SplitString(std::string Str, std::string By, std::vector<std::string>& Tokens) {
    Tokens.push_back(Str);
    const auto splitLen = By.size();
    while (true) {
        auto frag    = Tokens.back();
        const auto splitAt = frag.find(By);
        if (splitAt == std::string::npos)
            break;
        Tokens.back() = frag.substr(0, splitAt);
        Tokens.push_back(frag.substr(splitAt + splitLen));
    }
}

class CFilesys {
public:
    static void InitLib(lua_State* L);
};
inline auto FileLib = std::make_unique<CFilesys>();
