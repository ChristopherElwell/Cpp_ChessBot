#include <cstdint>
#include <array>
#include <utility>
#include <string>

namespace perft
{
    extern const std::array<std::pair<std::string, std::array<uint64_t, 6>>, 6> PERFT_DATA;
    void run_perft_test(int max_draft);
}

namespace test_positions
{
    void test_win_at_chess(int count);
}