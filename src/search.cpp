#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <limits>
#include <memory>
#include <print>
#include <thread>
#include <utility>

#include "bitboard.h"
#include "engine.h"
#include "eval.h"
#include "move.h"
#include "move_gen.h"

using namespace std;

/******************************************************************/

template <side_t Side>
constexpr int init_eval = 0;  // default (optional)

template <>
constexpr int init_eval<side_t::white> = numeric_limits<int>::min();

template <>
constexpr int init_eval<side_t::black> = numeric_limits<int>::max();

/******************************************************************/

namespace
{
template <side_t Side>
constexpr auto mate_eval(const MoveGen& move_gen, int ply) noexcept -> int
{
    if constexpr (Side == side_t::white)
    {
        if (move_gen.is_king_in_check<side_t::white>())
        {
            // checkmate for black
            return -checkmate_eval + ply;
        }
        // stalemate
        return 0;
    }
    else
    {
        if (move_gen.is_king_in_check<side_t::black>())
        {
            // checkmate for white
            return checkmate_eval - ply;
        }
        // stalemate
        return 0;
    }
}
}  // namespace

template <side_t Side>
void Engine::search_async(search_state& state)
{
    int depth_completed = 0;
    PVTable pv_completed = {};
    for (int depth = 1; !state.b_stop; depth++)
    {
        // No use of this eval
        search<Side>(search_args{.depth = depth, .alpha = alpha_init, .beta = beta_init, .ply = 0},
                     state);

        if (!state.b_stop)
        {
            depth_completed = depth;
            pv_completed = state.pv;
        }
    }
    LOG("Completed depth: {}", depth_completed);
    state.pv = pv_completed;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
template <side_t Side>
auto Engine::search(search_args args, search_state& state) -> int
{
    auto [depth, alpha, beta, ply] = args;
    auto& [board, b_stop, pv] = state;
    // instantly stop searching and cleanup
    if (b_stop.load(memory_order_relaxed))
    {
        return 0;
    }

    pv.clear(ply);

    // if end of iteration, return evaluation of board
    if (depth == 0)
    {
        return evaluate(board);
    }

    Move best_move = {};
    bool b_found_a_move = false;
    int best_eval = init_eval<Side>;

    auto move_gen = MoveGen(board);
    move_gen.gen<Side>();
    for (const auto& move : move_gen)
    {
        board.apply_move(move);

        // check if move leaves king in check
        if (move_gen.is_king_in_check<Side>())
        {
            board.apply_move(move);
            continue;
        }

        const int eval = search<~Side>(
            search_args{.depth = depth - 1, .alpha = alpha, .beta = beta, .ply = ply + 1}, state);
        board.apply_move(move);

        if constexpr (Side == side_t::white)
        {
            if (eval > best_eval)
            {
                best_eval = eval;
                best_move = move;
                b_found_a_move = true;
                pv.update(ply, best_move);
            }
            alpha = max(alpha, best_eval);
            if (alpha >= beta)
            {
                break;
            }
        }
        else
        {  // black
            if (eval < best_eval)
            {
                best_eval = eval;
                best_move = move;
                b_found_a_move = true;
                pv.update(ply, best_move);
            }
            beta = min(beta, best_eval);
            if (beta <= alpha)
            {
                break;
            }
        }
    }

    // If no moves, must be either stalemate or checkmate
    if (!b_found_a_move)
    {
        return mate_eval<Side>(move_gen, ply);
    }

    return best_eval;
}

template void Engine::search_async<side_t::white>(search_state& state);
template void Engine::search_async<side_t::black>(search_state& state);
template auto Engine::search<side_t::white>(search_args args, search_state& state) -> int;
template auto Engine::search<side_t::black>(search_args args, search_state& state) -> int;
