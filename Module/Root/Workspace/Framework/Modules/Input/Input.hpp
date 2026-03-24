#pragma once
#include "Includes.hpp"

class CInput {
public:
	static void InitLib(lua_State* L);
};
inline auto Input = std::make_unique<CInput>();