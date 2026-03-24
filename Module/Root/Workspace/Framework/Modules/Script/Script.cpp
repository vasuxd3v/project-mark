//
// Created by @binninwl_ on 12/06/2025.
//
#pragma once	
#include "Script.hpp"

int identifyexecutor(lua_State* L) {
	lua_pushstring(L, "Cobalt");
	lua_pushstring(L, "1.0.0");
	return 2;
}

int gethui(lua_State* L) {

    return 1;
}
int getscriptbytecode(lua_State* L)
{
    if (lua_type(L, 1) != LUA_TUSERDATA) { lua_pushnil(L); return 1; }

    uintptr_t script = *(uintptr_t*)lua_touserdata(L, 1);
    if (!script)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_getfield(L, 1, "ClassName");
    const char* name = lua_tostring(L, -1);
    lua_pop(L, 1);

    uintptr_t addr = name && strcmp(name, "ModuleScript") == 0
        ? *(uintptr_t*)(script + ModuleScriptOffset)
        : *(uintptr_t*)(script + LocalScriptOffset);

    std::string bytecode = ReadBytecode(addr);
    std::string code = addr ? DecompressBytecode(bytecode) : "";
    if (code.empty())
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushlstring(L, code.data(), code.size());
    return 1;
}


void CScript::InitLib(lua_State* L) {
	declare__Global(L, "identifyexecutor", identifyexecutor);
	declare__Global(L, "getexecutorname", identifyexecutor);
	declare__Global(L, "getscriptbytecode", getscriptbytecode);
}