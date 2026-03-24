#pragma once
#include "Includes.hpp"
#include <Framework/Environment.hpp>

class CTPService {
public:
	static void Initialize();
};
inline auto TPService = std::make_unique<CTPService>();