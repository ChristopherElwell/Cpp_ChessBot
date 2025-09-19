#include "bitboard.h"
#include "helpers.h"
#include "move_gen.h"
#include "testing.h"
#include <iostream>
#include <print>

using namespace std;

auto main() -> int {
  // perft::run_perft_test(2);
  auto board = BitBoard(
      "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
  auto moves = MoveGen(board);
  cout << board;
  moves.gen();
  // int i = 0;
  // for (const auto &move : moves) {
  //   board.apply_move(move);
  //   if (moves.is_black_king_in_check()) {
  //     board.apply_move(move);
  //     i++;
  //     continue;
  //   }
  //   auto inner_moves = MoveGen(board);
  //   inner_moves.gen();
  //   board.apply_move(move);
  //   print("{} {} {}\n", i++, helpers::move_to_uci(move, board),
  //         inner_moves.length());
  // }
}
