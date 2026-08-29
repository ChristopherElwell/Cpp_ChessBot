#include "testing.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <print>
#include <ranges>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "bitboard.h"
#include "engine.h"
#include "move.h"
#include "move_gen.h"
#include "private.h"

using namespace std;

namespace
{
const array<pair<string, array<uint64_t, 6>>, 6> perft_tests = {
    make_pair("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
              array<uint64_t, 6>{20, 400, 8902, 197281, 4865609, 119060324}),
    make_pair("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ",
              array<uint64_t, 6>{48, 2039, 97862, 4085603, 193690690, 8031647685}),
    make_pair("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 ",
              array<uint64_t, 6>{14, 191, 2812, 43238, 674624, 11030083}),
    make_pair("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
              array<uint64_t, 6>{6, 264, 9467, 422333, 15833292, 706045033}),
    make_pair("r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1 ",
              array<uint64_t, 6>{6, 264, 9467, 422333, 15833292, 706045033}),
    make_pair("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 "
              "10 ",
              array<uint64_t, 6>{46, 2079, 89890, 3894594, 164075551, 6923051137})};
auto read_csv(const filesystem::path &filename) -> vector<vector<string>>;
}  // namespace

template <side_t Side>
auto perft_search(BitBoard &board, int iter) -> uint64_t
{
    if (iter == 0)
    {
        return 1;
    }
    uint64_t perft = 0;

    auto move_gen = MoveGen(board);
    move_gen.gen<Side>();
    for (const Move &move : move_gen)
    {
        board.apply_move(move);
        // check if move leaves white king in check
        if (move_gen.is_king_in_check<Side>())
        {
            board.apply_move(move);
            continue;
        }
        perft += perft_search<~Side>(board, iter - 1);
        board.apply_move(move);
    }
    return perft;
}

void run_perft_test(int max_draft)
{
    int perft_test_counter = 1;
    int tests_passed = 0;
    for (auto [fen, correct_perfts] : perft_tests)
    {
        auto board = BitBoard(fen);
        print("\nRunning Perft Test {}\n{}\n", perft_test_counter++, fen);
        int drafts_passed = 0;
        for (int idx = 0; idx < max_draft; idx++)
        {
            uint64_t perft = 0;
            if (board.side_to_move() == side_t::white)
            {
                perft = perft_search<side_t::white>(board, idx + 1);
            }
            else
            {
                perft = perft_search<side_t::black>(board, idx + 1);
            }
            if (perft == correct_perfts.at(idx))
            {
                print("\tPassed, Draft: {}\n", idx + 1);
                drafts_passed++;
            }
            else
            {
                print("\tFailed, Draft: {} | Correct Perft: | This Perft: {}\n", idx + 1,
                      correct_perfts.at(idx), perft);
            }
        }
        if (drafts_passed == max_draft)
        {
            tests_passed++;
        }
    }
    print("Pass Rate: {}/{}\n", tests_passed, perft_tests.size());
}

void test_puzzles(size_t count)
{
    const vector<vector<string>> pzls = read_csv(priv::win_at_chess_file);
    size_t idx = 0;
    int passed = 0;
    Engine engine;
    count = min(count, pzls.size());
    chrono::milliseconds sum_time = {};
    for (auto pzl : pzls)
    {
        const string &fen = pzl[0];
        const string &answer = pzl[1];
        const string &pzl_id = pzl[2];
        if (idx++ >= count)
        {
            break;
        }
        LOG("\nRUNNING TESTS: {}\n{}\n", pzl_id, fen);

        const auto clock_start = chrono::high_resolution_clock::now();

        engine.load(fen);
        engine.run(6);
        const auto clock_end = chrono::high_resolution_clock::now();
        sum_time += chrono::duration_cast<chrono::milliseconds>(clock_end - clock_start);

        if (engine.get_algebraic() == answer)
        {
            LOG("PASSED [{}]", answer);
            passed++;
        }
        else
        {
            LOG("\nFAILED | Bot Move: [{}] Correct Move: [{}]\n", engine.get_algebraic(), answer);
        }
    }
    LOG("\n\nPASS RATE: {}/{}", passed, count);
    LOG("Time to complete: {}", sum_time);
}

void test_move_conversion()
{
    Engine engine;
    BitBoard board = BitBoard("r1b1kb2/pPppp1pp/n1p5/1q3pPr/8/2N5/1PP1PP2/R1BQK2R w KQq f6 0 1");
    vector<string> test_moves = {"e1g1",  "h1h5",  "a1a5", "a1a6", "b7a8q",
                                 "b7b8n", "b7c8b", "e1d2", "c1f4"};
    vector<string> post_move_fen = {"r1b1kb2/pPppp1pp/n1p5/1q3pPr/8/2N5/1PP1PP2/R1BQ1RK1 b q -",
                                    "r1b1kb2/pPppp1pp/n1p5/1q3pPR/8/2N5/1PP1PP2/R1BQK3 b Qq -",
                                    "r1b1kb2/pPppp1pp/n1p5/Rq3pPr/8/2N5/1PP1PP2/2BQK2R b Kq -",
                                    "r1b1kb2/pPppp1pp/R1p5/1q3pPr/8/2N5/1PP1PP2/2BQK2R b Kq -",
                                    "Q1b1kb2/p1ppp1pp/n1p5/1q3pPr/8/2N5/1PP1PP2/R1BQK2R b KQ -",
                                    "rNb1kb2/p1ppp1pp/n1p5/1q3pPr/8/2N5/1PP1PP2/R1BQK2R b KQq -",
                                    "r1B1kb2/p1ppp1pp/n1p5/1q3pPr/8/2N5/1PP1PP2/R1BQK2R b KQq -",
                                    "r1b1kb2/pPppp1pp/n1p5/1q3pPr/8/2N5/1PPKPP2/R1BQ3R b q -",
                                    "r1b1kb2/pPppp1pp/n1p5/1q3pPr/5B2/2N5/1PP1PP2/R2QK2R b KQq -"};
    int success = 0;
    for (const auto &fen : post_move_fen)
    {
        const auto fen_converted = BitBoard(fen).to_fen();
        if (fen_converted != fen)
        {
            println("Failed to encode and unencode: \n\t[{}]\n\t[{}]", fen, fen_converted);
        }
        else
        {
            success++;
        }
    }
    println("Fen conversion test complete. Success: {}/{}", success, post_move_fen.size());
    success = 0;
    for (const auto &[uci, fen] : ranges::views::zip(test_moves, post_move_fen))
    {
        BitBoard this_board = board;
        bool b_success = true;
        const Move mov = Engine::uci_to_move(uci, this_board);
        const string uci_converted = Engine::move_to_uci(mov, this_board);
        if (uci != uci_converted)
        {
            print("Failed to encode and unencode: [{}] -> [{}]", uci, uci_converted);
            b_success = false;
            continue;
        }
        this_board.apply_move(mov);
        const string fen_converted = this_board.to_fen();
        this_board.apply_move(mov);
        if (fen != fen_converted)
        {
            println(
                "Failed to encode and unencode with move [{}]: \n\tCorrect: [{}]\n\t  Wrong: [{}]",
                uci, fen, fen_converted);
            b_success = false;
        }
        success += b_success ? 1 : 0;
    }
    println("Move conversion test complete. Success: {}/{}", success, post_move_fen.size());
}

namespace
{
auto read_csv(const filesystem::path &filename) -> vector<vector<string>>
{
    vector<vector<string>> result;
    ifstream file(filename);

    if (!file.is_open())
    {
        LOG("Error opening file: {}. Error: {}", filename.string(), errno);
        std::println("filename: '{}'", filename.string());
        std::println("exists: {}", std::filesystem::exists(filename));
        std::println("absolute: {}", std::filesystem::absolute(filename).string());
        std::println("is regular: {}", std::filesystem::is_regular_file(filename));
        return result;
    }

    string line;
    while (getline(file, line))
    {
        vector<string> row;
        stringstream stream(line);
        string cell;

        while (getline(stream, cell, ','))
        {
            row.push_back(cell);
        }

        result.push_back(row);
    }

    return result;
}
}  // namespace
