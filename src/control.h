#pragma once
#include "search.h"
#include "helpers.h"
#include "move_gen.h"
#include <string>

namespace control
{
    std::string get_bot_move(std::string FEN, int max_depth);
    
} // namespace control
