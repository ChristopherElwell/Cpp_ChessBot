#include <cstdint>
#include <string>

#include "bitboard.h"

void run_perft_test(int max_draft);
template <side_t Side>
auto perft_search(BitBoard &board, int iter) -> uint64_t;
void test_puzzles(size_t count);
void test_move_conversion();
void test_zobrist_hash();
void test_board_history();
// Prints, for each legal move at `fen`, the perft node count of the subtree under
// that move ("divide"), followed by the total. Format is one "uci: count" line per
// move, a blank line, then "Nodes searched: N" -- meant to be diffed against a
// reference implementation to isolate exactly which move/position a move-gen bug
// lives in. See perft_debug.py.
void run_perft_divide(const std::string &fen, int depth);
// Applies `uci` to `fen` and compares the resulting board's Zobrist hash, computed
// two ways: incrementally (via apply_move, as perft actually uses it) vs freshly
// (by re-parsing the resulting FEN into a brand new BitBoard). A mismatch means
// apply_move is leaving some state out of sync -- useful when perft_debug.py's
// bisection bottoms out with "no discrepancy" on a fresh divide, which means the
// bug is in apply_move's incremental state update, not in move generation itself.
void debug_check_state_after_move(const std::string &fen, const std::string &uci);
// Walks the full perft tree from `fen` to `depth`, and after every single
// apply_move+undo_move round trip (at every node, not just the root), checks that
// the board's hash is back to exactly what it was before that move. Stops at the
// first move whose undo doesn't perfectly reverse its apply, printing the move and
// the full move path from the root that reached it. Use this when a fresh divide
// on a specific position matches the reference but the same position, reached
// via the normal apply/recurse/undo loop, doesn't -- that combination means
// undo_move (not move generation) is leaving residue for later sibling moves.
void run_perft_verify(const std::string &fen, int depth);
