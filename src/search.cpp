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
auto search(BitBoard &board, int iter, int alpha, int beta) -> SearchRes * {
  auto *this_result = new SearchRes();

  // if end of iteration, return evaluation of board
  if (iter == 0) {
    this_result->best_eval = evaluate(board);
    return this_result;
  }

  // initialize move array large enough for most possible moves in a chess
  // position (219) with head room (240) to account for varying piece promotions
  array<Move, MOVES_ARRAY_LENGTH> movs;
  int best_move_idx = -1;

  int idx = 0;
  auto move_gen = MoveGen(board);
  if (board.whites_turn()) { // white
    this_result->best_eval = INT_MIN;
    move_gen.gen();
    for (const auto &move : move_gen) {
      board.apply_move(move);

      // check if move leaves white king in check
      if (move_gen.is_white_king_in_check()) {
        board.apply_move(move);
        idx++;
        continue;
      }

      SearchRes *child_result = search(board, iter - 1, alpha, beta);
      board.apply_move(move);

      // alpha-beta pruning
      if (child_result->best_eval > this_result->best_eval) {
        helpers::free_search_result(
            this_result->best_result); // free old best move
        this_result->best_result =
            child_result; // set new best move to this one
        this_result->best_eval =
            child_result->best_eval; // set new best eval to this one
        best_move_idx = idx;         // set new best move ptr to this one
        if (this_result->best_eval >= beta) {
          break;
        }
        alpha = max(child_result->best_eval, alpha); // update alpha
      }

      // if this move was not chosen, free memory
      if (this_result->best_result != child_result) {
        helpers::free_search_result(child_result);
      }
      idx++;
    }
    // copy best move to search return
    if (best_move_idx != -1) {
      this_result->best_move = Move::copy(movs[best_move_idx]);
    } else { // if no valid move found we have either a checkmate or a stalemate
      // test if king is in check to determine checkmate vs stalemate, apply
      // checkmate eval function to incentivise checkmates as soon as possible
      // (prevents playing wiht the opponent forever)
      this_result->best_eval =
          (move_gen.is_white_king_in_check())
              ? (-(CHECKMATE_EVAL - (EARLY_CHECKMATE_INCENTIVE / (iter + 1))))
              : 0;
    }
  } else { // black
    this_result->best_eval = INT_MAX;
    move_gen.gen();
    for (const auto &move : move_gen) {
      if (move.type == movType::BOOK_END) {
        break;
      }
      board.apply_move(move);

      // check if move leaves white king in check
      if (move_gen.is_black_king_in_check()) {
        board.apply_move(move);
        idx++;
        continue;
      }

      SearchRes *child_result = search(board, iter - 1, alpha, beta);
      board.apply_move(move);

      if (child_result->best_eval < this_result->best_eval) {
        helpers::free_search_result(
            this_result->best_result); // free old best move
        this_result->best_result =
            child_result; // set new best move to this one
        this_result->best_eval =
            child_result->best_eval; // set new best eval to this one
        best_move_idx = idx;         // set new best move ptr to this one
        if (this_result->best_eval <= alpha) {
          break;
        }
        beta = min(child_result->best_eval, beta); // update beta
      }

      // alpha-beta pruning
      // if this move was not chosen, free memory
      if (this_result->best_result != child_result) {
        helpers::free_search_result(child_result);
      }
      idx++;
    }
    // copy best move to search return
    if (best_move_idx != -1) {
      this_result->best_move = Move::copy(movs[best_move_idx]);
    } else { // if no valid move found we have either a checkmate or a stalemate
      // test if king is in check to determine checkmate vs stalemate, apply
      // checkmate eval function to incentivise checkmates as soon as possible
      // (prevents playing wiht the opponent forever)
      this_result->best_eval =
          (move_gen.is_black_king_in_check())
              ? (CHECKMATE_EVAL - (EARLY_CHECKMATE_INCENTIVE / (iter + 1)))
              : 0;
    }
  }
  return this_result;
}
