#include "Yielder.hpp"

struct TaskDataStruct
{
	lua_State* state;
	std::function<CYielder::YielderReturn()> generator;
	void* work; // was PTP_WORK (Windows thread pool handle) — unused on macOS
};

void ThreadWorker(TaskDataStruct* data)
{
	try
	{
		auto yield_result = data->generator();	
		int result_count = yield_result(data->state);

		lua_State* thread_ctx = lua_newthread(data->state);

		lua_getglobal(thread_ctx, "task");
		lua_getfield(thread_ctx, -1, "defer");
		lua_pushthread(data->state);
		lua_xmove(data->state, thread_ctx, 1);
		lua_pop(data->state, 1);

		for (int i = result_count; i >= 1; --i)
		{
			lua_pushvalue(data->state, -i);
			lua_xmove(data->state, thread_ctx, 1);
		}

		lua_pcall(thread_ctx, result_count + 1, 0, 0);
		lua_settop(thread_ctx, 0);
	}
	catch (...)
	{
	}

	delete data;
}

int CYielder::YielderExecution(lua_State* L, const std::function<YielderReturn()>& generator)
{
	lua_pushthread(L);
	lua_ref(L, -1);
	lua_pop(L, 1);

	auto* task = new TaskDataStruct{ L, generator, nullptr };

	std::thread(ThreadWorker, task).detach();
	
	L->base = L->top;
	L->status = LUA_YIELD;
	L->ci->flags |= 1;
	return -1;
}