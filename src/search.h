#pragma once

#include "move.h"
#include "move_gen.h"
#include <cstdint>

struct SearchRes{
    Move best_move;
    int16_t best_eval;
    SearchRes* best_result;

    SearchRes(const Move& mov, int16_t best_eval, SearchRes* best_result) : 
        best_move(mov), best_eval(best_eval), best_result(best_result) {};

    SearchRes() = default;
};

SearchRes* search(BitBoard& bb, MoveGen& mg, int iter, int16_t alpha, int16_t beta);