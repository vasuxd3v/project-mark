#include "Execution.hpp"

class BytecodeEncoder : public Luau::BytecodeEncoder
{
	inline void encode(uint32_t* data, size_t count) override
	{
		for (auto i = 0; i < count;)
		{
			uint8_t Opcode = LUAU_INSN_OP(data[i]);
			const auto LookupTable = reinterpret_cast<BYTE*>(Offsets::Lua::OpcodeLookupTable);
			uint8_t FinalOpcode = Opcode * 227;
			FinalOpcode = LookupTable[FinalOpcode];

			data[i] = (FinalOpcode) | (data[i] & ~0xFF);
			i += Luau::getOpLength(static_cast<LuauOpcode>(Opcode));
		}
	}
};

std::string CExecution::Compile(std::string Source)
{
	auto bytecodeEncoder = BytecodeEncoder();
	static const char* CommonGlobals[] = { "Game", "Workspace", "game", "plugin", "script", "shared", "workspace", "_G", "_ENV", nullptr };

	Luau::CompileOptions Options;
	Options.debugLevel = 1;
	Options.optimizationLevel = 1;
	Options.mutableGlobals = CommonGlobals;
	Options.vectorLib = "Vector3";
	Options.vectorCtor = "new";
	Options.vectorType = "Vector3";

	return Luau::compile(Source, Options, {}, &bytecodeEncoder);
}

lua_State* CExecution::NewThread(lua_State* L) {
	if (L == nullptr || L->tt != LUA_TTHREAD)
		return nullptr;

	luaC_checkGC(L);
	luaC_threadbarrier(L);

	int StackCount = lua_gettop(L);
	if (StackCount > 0)
		lua_pop(L, StackCount);

	lua_State* L1 = luaE_newthread(L);
	if (L1 == nullptr)
		return nullptr;

	if (L1->tt != LUA_TTHREAD)
		return nullptr;

	setthvalue(L, L->top, L1);
	incr_top(L);

	global_State* G = L->global;
	if (G->cb.userthread != nullptr)
		G->cb.userthread(L, L1);

	return L1;
}

void CExecution::SendScript(lua_State* L, std::string Script)
{
	if (Script.empty()) return;

	auto Top = lua_gettop(L);
	auto Thread = lua_newthread(L);
	lua_pop(L, 1);
	luaL_sandboxthread(Thread);
	Scheduler->SetThreadCaps(Thread, 8, Capabilities);

	auto bytecode = CExecution::Compile(Script);
	if (bytecode.empty()) return;

	if (luau_load(Thread, "@Cobalt", bytecode.data(), bytecode.size(), 0) != LUA_OK) {
		auto err = lua_tostring(Thread, -1);
		ROBLOX::Print(3, "%s", err);
		lua_pop(Thread, 1);
		lua_settop(L, Top);
		return;
	}

	auto closure = (Closure*)(lua_topointer(Thread, -1));
	if (closure && closure->l.p)
		Scheduler->SetProtoCaps(closure->l.p, &Capabilities);

	lua_getglobal(L, "task");
	lua_getfield(L, -1, "defer");
	lua_remove(L, -2);
	lua_xmove(Thread, L, 1);

	if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
		auto err = lua_tostring(L, -1);
		ROBLOX::Print(3, "%s", err);
		lua_pop(L, 1);
	}

	lua_settop(Thread, 0);
	lua_settop(L, Top);
}

