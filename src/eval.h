#ifndef EVALUATE_H
#define EVALUATE_H

#include "bitboard.h"

namespace
{

constexpr int16_t checkmate_eval = 30000;
constexpr int16_t max_ply = 200;
constexpr int16_t mate_threshold = checkmate_eval - max_ply;
}  // namespace

template <side_t Side>
auto evaluate(const BitBoard &board) -> int16_t;

auto is_mate_eval(int16_t eval) -> bool;

#endif
