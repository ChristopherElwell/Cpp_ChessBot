#pragma once

#include "bitboard.h"

static constexpr int checkmate_eval = 30000;
static constexpr int early_checkmate_incentive = 2000;

template <side_t Side>
auto evaluate(const BitBoard &board) -> int;
