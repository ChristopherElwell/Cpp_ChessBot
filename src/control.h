#pragma once
#include <string>

namespace control
{
auto get_bot_move_at_depth_uci(std::string FEN, int max_depth) -> std::string;
auto get_bot_move_at_depth_algebreic(const std::string &FEN, int max_depth) -> std::string;
void receiver();
} // namespace control
