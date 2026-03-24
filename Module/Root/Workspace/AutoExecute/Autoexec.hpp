#pragma once 
#include "Includes.hpp"

class CAutoExecute {
public:
    static void Run();
};
inline auto AutoExecution = std::make_unique<CAutoExecute>();