#include "search.h"
#include "bitboard.h"
#include "data.h"
#include "eval.h"
#include "helpers.h"
#include "move.h"
#include "move_gen.h"
#include <algorithm>
#include <array>
#include <climits>

using namespace std;

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto search(BitBoard &board, int iter, int alpha, int beta) -> SearchRes *
{
    auto *this_result = new SearchRes();

    // if end of iteration, return evaluation of board
    if (iter == 0)
    {
        this_result->best_eval = evaluate(board);
        return this_result;
    }

    // initialize move array large enough for most possible moves in a chess
    // position (219) with head room (240) to account for varying piece promotions
    int best_move_idx = -1;

    int idx = 0;
    auto move_gen = MoveGen(board);
    if (board.whites_turn())
    { // white
        this_result->best_eval = INT_MIN;
        move_gen.gen();
        for (const auto &move : move_gen)
        {
            board.apply_move(move);

            // check if move leaves white king in check
            if (move_gen.is_white_king_in_check())
            {
                board.apply_move(move);
                idx++;
                continue;
            }

            SearchRes *child_result = search(board, iter - 1, alpha, beta);
            board.apply_move(move);

            // alpha-beta pruning
            if (child_result->best_eval > this_result->best_eval)
            {
                helpers::free_search_result(this_result->best_result);
                this_result->best_result = child_result;
                this_result->best_eval = child_result->best_eval;
                best_move_idx = idx;
                if (this_result->best_eval >= beta)
                {
                    break;
                }
                alpha = max(child_result->best_eval, alpha);
            }
            if (this_result->best_result != child_result)
            {
                helpers::free_search_result(child_result);
            }
            idx++;
        }
        // copy best move to search return
        if (best_move_idx != -1)
        {
            this_result->best_move = Move::copy(move_gen.get(best_move_idx));
        }
        else
        {
            this_result->best_eval =
                (move_gen.is_white_king_in_check())
                    ? (-(CHECKMATE_EVAL - (EARLY_CHECKMATE_INCENTIVE / (iter + 1))))
                    : 0;
        }
    }
    else
    { // black
        this_result->best_eval = INT_MAX;
        move_gen.gen();
        for (const auto &move : move_gen)
        {
            if (move.type == movType::BOOK_END)
            {
                break;
            }
            board.apply_move(move);

            // check if move leaves white king in check
            if (move_gen.is_black_king_in_check())
            {
                board.apply_move(move);
                idx++;
                continue;
            }

            SearchRes *child_result = search(board, iter - 1, alpha, beta);
            board.apply_move(move);

            if (child_result->best_eval < this_result->best_eval)
            {
                helpers::free_search_result(this_result->best_result);
                this_result->best_result = child_result;
                this_result->best_eval = child_result->best_eval;
                best_move_idx = idx;
                if (this_result->best_eval <= alpha)
                {
                    break;
                }
                beta = min(child_result->best_eval, beta); // update beta
            }

            // alpha-beta pruning
            if (this_result->best_result != child_result)
            {
                helpers::free_search_result(child_result);
            }
            idx++;
        }
        if (best_move_idx != -1)
        {
            this_result->best_move = Move::copy(move_gen.get(best_move_idx));
        }
        else
        {
            this_result->best_eval =
                (move_gen.is_black_king_in_check())
                    ? (CHECKMATE_EVAL - (EARLY_CHECKMATE_INCENTIVE / (iter + 1)))
                    : 0;
        }
    }
    return this_result;
}
