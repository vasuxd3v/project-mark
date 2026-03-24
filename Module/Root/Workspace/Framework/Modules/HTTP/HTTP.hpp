#pragma once
#include <Includes.hpp>
#include <Framework/Environment.hpp>
class CHTTP {
public:
	static void InitLib(lua_State* L);
	static int HTTPGet(lua_State* L);
	static int GetObjects(lua_State* L);
};
inline auto HTTPLibrary = std::make_unique<CHTTP>();