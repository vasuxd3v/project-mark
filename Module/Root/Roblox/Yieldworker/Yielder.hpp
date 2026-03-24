#pragma once
#include "Includes.hpp"

class CYielder {
	public:
		using YielderReturn = std::function<int(lua_State* L)>;
	static int YielderExecution(lua_State* L, const std::function<YielderReturn()>& generator);
};	
inline auto Yielder = std::make_unique<CYielder>();