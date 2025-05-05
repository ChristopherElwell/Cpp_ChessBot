#pragma once
#include "search.h"
#include "helpers.h"
#include "move_gen.h"
#include <string>

namespace control
{
    std::string get_bot_move_at_depth_uci(std::string FEN, int max_depth);
    std::string get_bot_move_at_depth_algebreic(std::string FEN, int max_depth);
    void receiver();
} // namespace control
