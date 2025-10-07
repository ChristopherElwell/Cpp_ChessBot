#include "bitboard.h"
#include "control.h"
#include "helpers.h"
#include "move_gen.h"
#include "testing.h"
#include <iostream>
#include <print>

using namespace std;

auto main() -> int
{
    test_positions::test_win_at_chess(5);
    // BitBoard board = BitBoard::start_position();
    // SearchRes *result = search(board, 1, INT_MIN, INT_MAX);
    // print("DONE!");
    // print("{}, \n{}\n", result->best_move.to_string(),
    //       helpers::move_to_uci(result->best_move, board));
    // result = search(board, 2, INT_MIN, INT_MAX);
    // print("DONE!");
    // print("{}, \n{}\n", result->best_move.to_string(),
    //       helpers::move_to_uci(result->best_move, board));
    // result = search(board, 3, INT_MIN, INT_MAX);
    // print("DONE!");
    // print("{}, \n{}\n", result->best_move.to_string(),
    //       helpers::move_to_uci(result->best_move, board));
    // result = search(board, 4, INT_MIN, INT_MAX);
    // print("DONE!");
    // print("{}, \n{}\n", result->best_move.to_string(),
    //       helpers::move_to_uci(result->best_move, board));
    // result = search(board, 5, INT_MIN, INT_MAX);
    // print("DONE!");
    // print("{}, \n{}\n", result->best_move.to_string(),
    //       helpers::move_to_uci(result->best_move, board));
}
