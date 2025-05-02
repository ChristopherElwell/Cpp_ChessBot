#include "bitboard.h"
#include "move_gen.h"
#include <utility>
#include <string>

namespace tests {
    extern const std::array<std::pair<std::string, std::array<uint64_t, 6>>, 6> PERFT_DATA;
    void run_perft_test(int max_draft);
    uint64_t perft_search_declare(std::string FEN, int iter);
}