#pragma once
#include "Includes.hpp"

class CExecution {
	public:
	static void SendScript(lua_State* L, std::string Script);
	static std::string Compile(std::string Source);
	static lua_State* NewThread(lua_State* L);
};
inline auto Execution = std::make_unique<CExecution>();