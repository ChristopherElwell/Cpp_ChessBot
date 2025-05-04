#pragma once
#include "search.h"
#include "helpers.h"
#include "move_gen.h"
#include <string>

namespace control
{
    std::string get_bot_move_at_depth(std::string FEN, int max_depth);
    void receiver();
} // namespace control
