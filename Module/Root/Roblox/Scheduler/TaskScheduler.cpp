#pragma once
#include "Includes.hpp"
#include "Scheduling.hpp"
#include <Framework/Environment.hpp>
#include <Communications/Communications.hpp>

using Return = std::function<int(lua_State* L)>;
struct Data
{
	lua_State* L;
	std::function<Return()> generator;
	void* work; // was PTP_WORK (Windows thread pool handle) — unused on macOS
};

bool CTaskScheduler::Init()
{
    uintptr_t DM = Scheduler->DataModel();
    uintptr_t SC = Scheduler->ScriptContext(DM);
    lua_State* RobloxState = Scheduler->LuaState(SC);

    Globals::ExecutorThread = lua_newthread(RobloxState);
    Scheduler->SetThreadCaps(Globals::ExecutorThread, 8, Capabilities);
    Environment->Initialize(Globals::ExecutorThread);

    ROBLOX::Print(1, ("DataModel: 0x" + std::to_string(DM)).c_str());
    ROBLOX::Print(1, ("ScriptContext: 0x" + std::to_string(SC)).c_str());
    ROBLOX::Print(1, ("LuaState: 0x" + std::to_string(reinterpret_cast<uintptr_t>(RobloxState))).c_str());

    Communication->Initialize();

    return true;
}

