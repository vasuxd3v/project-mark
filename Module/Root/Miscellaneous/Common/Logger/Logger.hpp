// Common/Logger/Logger.hpp — macOS port
// Interface is IDENTICAL to the Windows version.
// Only the implementation (Logger.cpp) changes.

#pragma once
#include "Includes.hpp"
#include <fstream>
#include <filesystem>

namespace Logger
{
    extern std::ofstream logFile;
    extern std::filesystem::path dllDir;

    void printf(const char* fmt, ...);

    void Init();
}
