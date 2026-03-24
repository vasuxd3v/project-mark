//
// Created by @binninwl_ on 12/06/2025.
//

#include "Filesystem.hpp"

int writefile(lua_State* L) {

    const char* path = luaL_checkstring(L, 1);
    size_t len;
    const char* data = luaL_checklstring(L, 2, &len);

    std::string fullPath = WorkspaceDirectory() + path;
    std::ofstream file(fullPath, std::ios::binary);
    if (file.is_open()) file.write(data, len);
    return 0;
}

int readfile(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    std::string fullPath = WorkspaceDirectory() + path;

    if (!std::filesystem::is_regular_file(fullPath)) {
        luaL_error(L, "File does not exist: %s", fullPath.c_str());
        return 0;
    }
    std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        lua_pushnil(L);
        return 1;
    }

    std::streamsize size = file.tellg();
    file.seekg(0);
    std::string buffer(size, '\0');
    file.read(&buffer[0], size);
    lua_pushlstring(L, buffer.c_str(), buffer.size());
    return 1;
}

int makefolder(lua_State* L) {
    luaL_checktype(L, 1, LUA_TSTRING);
    std::string Path = luaL_checklstring(L, 1, 0);

    std::replace(Path.begin(), Path.end(), '\\', '/');
    std::vector<std::string> Tokens;
    _SplitString(Path, "/", Tokens);

    std::string CurrentPath = WorkspaceDirectory();
    std::replace(CurrentPath.begin(), CurrentPath.end(), '\\', '/');

    for (const auto& Token : Tokens) {
        CurrentPath += Token + "/";
        if (!std::filesystem::is_directory(CurrentPath))
            std::filesystem::create_directory(CurrentPath);
    }
    return 0;
}

int isfile(lua_State* L) {
    luaL_checktype(L, 1, LUA_TSTRING);
    std::string FullPath = WorkspaceDirectory() + luaL_checklstring(L, 1, 0);
    std::replace(FullPath.begin(), FullPath.end(), '\\', '/');
    lua_pushboolean(L, std::filesystem::is_regular_file(FullPath));
    return 1;
}

int delfile(lua_State* L) {
    luaL_checktype(L, 1, LUA_TSTRING);
    std::string FullPath = WorkspaceDirectory() + luaL_checklstring(L, 1, 0);
    std::replace(FullPath.begin(), FullPath.end(), '\\', '/');
    std::error_code ec;
    bool removed = std::filesystem::remove(FullPath, ec);
    lua_pushboolean(L, removed);
    return 1;
}

int listfiles(lua_State* L) {
    luaL_checktype(L, 1, LUA_TSTRING);
    std::string Path = WorkspaceDirectory() + luaL_checklstring(L, 1, 0);
    std::replace(Path.begin(), Path.end(), '\\', '/');

    if (!std::filesystem::is_directory(Path)) {
        lua_newtable(L);
        return 1;
    }

    lua_createtable(L, 0, 0);
    int i = 1;
    for (const auto& entry : std::filesystem::directory_iterator(Path)) {
        std::string rel = entry.path().string().substr(WorkspaceDirectory().length());
        lua_pushinteger(L, i);
        lua_pushstring(L, rel.c_str());
        lua_settable(L, -3);
        i++;
    }
    return 1;
}

int isfolder(lua_State* L) {
    luaL_checktype(L, 1, LUA_TSTRING);
    std::string FullPath = WorkspaceDirectory() + luaL_checklstring(L, 1, 0);
    std::replace(FullPath.begin(), FullPath.end(), '\\', '/');
    lua_pushboolean(L, std::filesystem::is_directory(FullPath));
    return 1;
}

int delfolder(lua_State* L) {
    luaL_checktype(L, 1, LUA_TSTRING);
    std::string FullPath = WorkspaceDirectory() + luaL_checklstring(L, 1, 0);
    std::replace(FullPath.begin(), FullPath.end(), '\\', '/');
    std::filesystem::remove_all(FullPath);
    return 0;
}

int getcustomasset(lua_State* L) {
    luaL_checktype(L, 1, LUA_TSTRING);
    std::string assetPath = WorkspaceDirectory() + lua_tostring(L, 1);
    std::replace(assetPath.begin(), assetPath.end(), '\\', '/');
    std::filesystem::path FullPath = assetPath;

    if (!std::filesystem::is_regular_file(FullPath)) {
        lua_pushnil(L);
        return 1;
    }

    auto customAssetsDir = std::filesystem::current_path() / "ExtraContent" / "Cobalt";
    auto customAssetsFile = customAssetsDir / FullPath.filename();

    if (!std::filesystem::exists(customAssetsDir))
        std::filesystem::create_directory(customAssetsDir);

    std::filesystem::copy_file(FullPath, customAssetsFile, std::filesystem::copy_options::update_existing);

    std::string Final = "rbxasset://Cobalt/" + customAssetsFile.filename().string();
    lua_pushlstring(L, Final.c_str(), Final.size());
    return 1;
}

int appendfile(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    size_t len;
    const char* data = luaL_checklstring(L, 2, &len);

    std::string fullPath = WorkspaceDirectory() + path;
    std::ofstream file(fullPath, std::ios::binary | std::ios::app);
    if (file.is_open()) {
        file.write(data, len);
    }
    return 0;
}

int dofile(lua_State* L) {
    if (!lua_isstring(L, 1))
        return 0;

    const char* path = lua_tostring(L, 1);
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return 0;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::string bytecode = Execution->Compile(content);
    int loadStatus = luau_load(L, path, bytecode.c_str(), bytecode.size(), 0);
    if (loadStatus != 0)
        return 0;
    lua_pcall(L, 0, LUA_MULTRET, 0);
    return lua_gettop(L);
}

int loadfile(lua_State* L) {
    if (!lua_isstring(L, 1))
        return 0;

    const char* path = lua_tostring(L, 1);
    std::string fullPath = WorkspaceDirectory() + path;

    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        lua_pushnil(L);
        return 1;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    std::string bytecode = Execution->Compile(content);

    int loadStatus = luau_load(L, path, bytecode.c_str(), bytecode.size(), 0);
    if (loadStatus != 0) {
        lua_pushnil(L);
        lua_insert(L, -2);
        return 2;
    }

    return 1;
}

void CFilesys::InitLib(lua_State* L) {
    declare__Global(L, "writefile", writefile);
    declare__Global(L, "readfile", readfile);
    declare__Global(L, "isfile", isfile);
    declare__Global(L, "delfile", delfile);
    declare__Global(L, "listfiles", listfiles);
    declare__Global(L, "makefolder", makefolder);
    declare__Global(L, "isfolder", isfolder);
    declare__Global(L, "delfolder", delfolder);
    declare__Global(L, "getcustomasset", getcustomasset);
    declare__Global(L, "appendfile", appendfile);
    declare__Global(L, "dofile", dofile);
    declare__Global(L, "loadfile", loadfile);
}