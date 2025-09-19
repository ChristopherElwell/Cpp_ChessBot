#pragma once

#include "bitboard.h"

static constexpr int CHECKMATE_EVAL = 30000;
static constexpr int EARLY_CHECKMATE_INCENTIVE = 2000;

auto evaluate(const BitBoard &board) -> int;