#pragma once
#include "Includes.hpp"
#include "Modules/Script/Script.hpp"
#include "Modules/HTTP/HTTP.hpp"
#include "Modules/FileLib/Filesystem.hpp"
#include "Modules/Input/Input.hpp"
#include "Modules/Closures/Closures.hpp"
class CEnvironment
{
public:
	static void Initialize(lua_State* L);
};
inline auto Environment = std::make_unique<CEnvironment>();
inline void declare__Global(lua_State* L, const char* Name, lua_CFunction Function) { lua_pushcclosure(L, Function, nullptr, 0); lua_setglobal(L, Name); }
inline void declare__Tablemember(lua_State* L, const char* Name, lua_CFunction Function) { lua_pushcclosure(L, Function, nullptr, 0); lua_setfield(L, -2, Name); }
static void SetNewIdentity(lua_State* L, int Identity) {
	L->userdata->Identity = Identity;
	std::int64_t Ignore[128];
	//ROBLOX::Impersonator(Ignore, &Identity, *(__int64*)((uintptr_t)L->userdata + Offsets::External::UserData::Capabilities));
}