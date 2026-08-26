#pragma once

#include "bitboard.h"

static constexpr int checkmate_eval = 30000;
static constexpr int early_checkmate_incentive = 2000;
static constexpr int beta_init = 1'000'000;
static constexpr int alpha_init = -beta_init;

auto evaluate(const BitBoard &board) -> int;
