#pragma once

#include "bitboard.h"
#include "move.h"

struct SearchRes
{
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    Move best_move;
    int best_eval;
    SearchRes *best_result;
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    SearchRes() : best_eval(0), best_result(nullptr) {};
    SearchRes(const Move &mov, int best_eval, SearchRes *best_result)
        : best_move(mov), best_eval(best_eval), best_result(best_result) {};
};

auto search(BitBoard &board, int iter, int alpha, int beta) -> SearchRes *;