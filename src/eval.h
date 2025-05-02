#pragma once

#include "bitboard.h"
#include <cstdint>

inline constexpr int16_t CHECKMATE_EVAL = 30000;
inline constexpr int16_t EARLY_CHECKMATE_INCENTIVE = 2000;

int16_t evaluate(const BitBoard& bb);