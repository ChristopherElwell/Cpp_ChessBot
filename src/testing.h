#include <array>
#include <cstdint>
#include <string>
#include <utility>

#include "bitboard.h"

namespace perft {
extern const std::array<std::pair<std::string, std::array<uint64_t, 6>>, 6>
    PERFT_DATA;
void run_perft_test(int max_draft);
auto perft_search(BitBoard &board, int iter) -> uint64_t;
} // namespace perft

namespace test_positions {
void test_win_at_chess(int count);
}