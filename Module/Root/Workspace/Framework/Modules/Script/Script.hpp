#pragma once
#include <Includes.hpp>
#include <Framework/Environment.hpp>
class CScript {
	public:
	static void InitLib(lua_State* L);
};
inline auto ScriptLibrary = std::make_unique<CScript>();