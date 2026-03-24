#include "Environment.hpp"

void CEnvironment::Initialize(lua_State* L) {

	CScript::InitLib(L);
    CHTTP::InitLib(L);
    CFilesys::InitLib(L);
    CInput::InitLib(L);
    CClosures::InitLib(L);

    lua_newtable(L);
    lua_setglobal(L, "_G");

    lua_newtable(L);
    lua_setglobal(L, "shared");
}
