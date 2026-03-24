#pragma once
#include <Includes.hpp>
#include <Framework/Environment.hpp>
class CClosures {
public:
	static void InitLib(lua_State* L);
};
inline auto ClosuresLibrary = std::make_unique<CClosures>();