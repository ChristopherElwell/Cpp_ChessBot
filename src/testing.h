#include <cstdint>

#include "bitboard.h"

void run_perft_test(int max_draft);
template <side_t Side>
auto perft_search(BitBoard &board, int iter) -> uint64_t;
void test_puzzles(size_t count);
void test_move_conversion();
void test_zobrist_hash();
