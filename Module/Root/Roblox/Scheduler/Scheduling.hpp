// Roblox/Scheduler/Scheduling.hpp — macOS port
// Interface is IDENTICAL to the Windows version.
// The implementation file (Scheduler.cpp) changes IsValidPointer().
//
// __int64 is an MSVC type. On macOS we define it as int64_t in Includes.hpp.

#pragma once
#include <Includes.hpp>
#include <Globals.hpp>
inline uintptr_t PreviousDM;

class CScheduler {
public:
    static void SetProtoCaps(Proto* Proto, uintptr_t* Capabilities);
    static void SetThreadCaps(lua_State* L, int Level, uintptr_t Capabilities);
    static uintptr_t ScriptContext(uintptr_t DataModel);
    static uintptr_t DataModel();
    static lua_State* LuaState(uintptr_t ScriptContext);
    static bool GameIsLoaded(uintptr_t DataModel);
    static bool IsValidPointer(void* ptr, size_t size);
    static int64_t GetPlaceID();           // was __int64 (MSVC-only type)
    static uintptr_t GetJobByName(const std::string& Name);
    static void UpdateJobs();
};

class CTaskScheduler {
public:
    static bool Init();
};

inline auto Scheduler    = std::make_unique<CScheduler>();
inline auto TaskScheduler = std::make_unique<CTaskScheduler>();
