#include <cstdint>

#include "bitboard.h"

void run_perft_test(int max_draft);
template <side_t side>
auto perft_search(BitBoard &board, int iter) -> uint64_t;
void test_puzzles(size_t count);
