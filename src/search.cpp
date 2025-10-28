#include <algorithm>
#include <atomic>
#include <limits>
#include <memory>
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
constexpr auto no_moves_eval(const MoveGen& move_gen, int iter) noexcept -> int
{
    if constexpr (Side == side_t::white)
    {
        return move_gen.is_king_in_check<side_t::white>()
                   ? -(checkmate_eval - (early_checkmate_incentive / (iter + 1)))
                   : 0;
    }
    else
    {
        return move_gen.is_king_in_check<side_t::black>()
                   ? (checkmate_eval - (early_checkmate_incentive / (iter + 1)))
                   : 0;
    }
}
}  // namespace

template <side_t Side>
void Engine::search_async(pair<int, unique_ptr<const result_t>>& result, BitBoard board,
                          atomic_bool& b_stop)
{
    pair<int, unique_ptr<const result_t>> temp_result = {init_eval<Side>, nullptr};
    for (int depth = 1; !b_stop; depth++)
    {
        temp_result = search<Side>(board, depth, numeric_limits<int>::min(),
                                   numeric_limits<int>::max(), b_stop);

        if (!b_stop && temp_result.second)
        {
            result = std::move(temp_result);
        }
    }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
template <side_t Side>
auto Engine::search(BitBoard& board, int iter, int alpha, int beta, atomic_bool& b_stop)
    -> pair<int, unique_ptr<result_t>>
{
    // instantly stop searching and cleanup
    if (b_stop)
    {
        return {init_eval<~Side>, nullptr};
    }
    // if end of iteration, return evaluation of board
    if (iter == 0)
    {
        return {evaluate(board), nullptr};
    }

    auto p_result = make_unique<result_t>();
    auto move_gen = MoveGen(board);
    const Move* p_best_move = nullptr;
    int best_eval = init_eval<Side>;
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

        auto [eval, child_result] = search<~Side>(board, iter - 1, alpha, beta, b_stop);
        board.apply_move(move);

        if constexpr (Side == side_t::white)
        {
            if (eval > best_eval)
            {
                best_eval = eval;
                p_best_move = &move;
                p_result->next = std::move(child_result);
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
                p_best_move = &move;
                p_result->next = std::move(child_result);
            }
            beta = min(beta, best_eval);
            if (beta <= alpha)
            {
                break;
            }
        }
    }
    if (!p_best_move)
    {
        return {no_moves_eval<Side>(move_gen, iter), nullptr};
    }
    p_result->best_move = *p_best_move;
    return {best_eval, std::move(p_result)};
}

template void Engine::search_async<side_t::white>(pair<int, unique_ptr<const result_t>>& result,
                                                  BitBoard board, atomic_bool& b_stop);
template void Engine::search_async<side_t::black>(pair<int, unique_ptr<const result_t>>& result,
                                                  BitBoard board, atomic_bool& b_stop);
template auto Engine::search<side_t::white>(BitBoard&, int, int, int, atomic_bool& b_stop)
    -> std::pair<int, std::unique_ptr<result_t>>;
template auto Engine::search<side_t::black>(BitBoard&, int, int, int, atomic_bool& b_stop)
    -> std::pair<int, std::unique_ptr<result_t>>;