#pragma once
#include <thread>
#include <cstdint>
#include "Engine.hpp"
#include <string>
#include <Execution/Execution.hpp>
class CCommunications {
private:

public:
	static void Initialize();
	static void Setup();
};

inline auto Communication = std::make_unique<CCommunications>();