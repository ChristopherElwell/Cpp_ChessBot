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
void run_perft_divide(const std::string &fen, int depth);
void debug_check_state_after_move(const std::string &fen, const std::string &uci);
void run_perft_verify(const std::string &fen, int depth);
void test_ttable();
