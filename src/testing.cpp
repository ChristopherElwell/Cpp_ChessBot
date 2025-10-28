#include "testing.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <print>
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
auto read_csv(const string &filename) -> vector<vector<string>>;
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
            if (board.whites_turn())
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
    const vector<vector<string>> pzls = read_csv(priv::WIN_AT_CHESS_FILE);
    size_t idx = 0;
    int passed = 0;
    Engine engine;
    count = min(count, pzls.size());
    for (auto pzl : pzls)
    {
        const string &fen = pzl[0];
        const string &answer = pzl[1];
        const string &pzl_id = pzl[2];
        if (idx++ >= count)
        {
            break;
        }
        cout << "\n\nRUNNING TESTS: " << pzl_id << "\n" << fen << "\n";
        engine.load(fen);
        engine.run(6);
        print("run done\n");
        if (engine.get_algebraic() == answer)
        {
            cout << "\nPASSED [" << answer << "]\n";
            passed++;
        }
        else
        {
            print("\nFAILED | Bot Move: [{}] Correct Move: [{}]\n", engine.get_algebraic(), answer);
        }
    }
    cout << "\n\nPASS RATE: " << passed << "/" << count;
}

namespace
{
auto read_csv(const string &filename) -> vector<vector<string>>
{
    vector<vector<string>> result;
    ifstream file(filename);

    if (!file.is_open())
    {
        cerr << "Error opening file\n";
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