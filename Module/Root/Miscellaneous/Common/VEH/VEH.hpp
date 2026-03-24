// Common/VEH/VEH.hpp — macOS port
// Same interface as the Windows version — only the implementation changes.
// The class name CVEH is kept identical so all callers compile without changes.

#pragma once
#include "Includes.hpp"

class CVEH
{
public:
    // Start() — same signature as Windows.
    // Windows: registers SetUnhandledExceptionFilter(Alert)
    // macOS:   registers sigaction handlers for SIGSEGV, SIGBUS, SIGABRT, SIGILL
    static auto Start() -> bool;
    CVEH() = default;
};

inline auto VEH = std::make_unique<CVEH>();
