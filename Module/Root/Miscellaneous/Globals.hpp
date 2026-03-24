#pragma once
#include <Includes.hpp>

inline uintptr_t Capabilities = 0xFFFFFFFFFFFFFFFF; // 0xFFFFFFFFFFFFFFFF

namespace Globals {
    inline lua_State* RobloxThread;
    inline lua_State* ExecutorThread;
    inline bool Initialized = false;
    inline std::queue<std::function<void()>> YIELDPackets;
}