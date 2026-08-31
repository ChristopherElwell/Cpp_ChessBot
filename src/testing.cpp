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
#include "board_history.h"
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
    println("Running perft test at {} draft...", max_draft);
    int tests_passed = 0;
    for (auto [fen, correct_perfts] : perft_tests)
    {
        auto board = BitBoard(fen);
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
    println("Perft test complete\nPass rate: {:.0f}%\n",
            static_cast<float>(tests_passed) / static_cast<float>(perft_tests.size()) * 100);
}

void test_puzzles(size_t count)
{
    println("Running puzzle test. {} puzzles...", count);
    const vector<vector<string>> pzls = read_csv(priv::win_at_chess_path);
    size_t idx = 0;
    int passed = 0;
    Engine engine;
    count = min(count, pzls.size());
    chrono::milliseconds sum_time = {};
    for (auto pzl : pzls)
    {
        const string &fen = pzl[0];
        const string &answer = pzl[1];
        if (idx++ >= count)
        {
            break;
        }

        const auto clock_start = chrono::steady_clock::now();

        engine.load(fen);
        future<void> future = engine.run(6);
        future.get();
        const auto clock_end = chrono::high_resolution_clock::now();
        sum_time += chrono::duration_cast<chrono::milliseconds>(clock_end - clock_start);

        if (engine.get_algebraic() == answer)
        {
            passed++;
        }
        else
        {
            LOG("\nFAILED | Bot Move: [{}] Correct Move: [{}]\n", engine.get_algebraic(), answer);
        }
    }
    println("Puzzle test complete\nPass rate: {:.0f}%\nTime to complete: {}\n",
            static_cast<float>(passed) / static_cast<float>(count) * 100, sum_time);
}

void test_move_conversion()
{
    println("Runnning move conversion test...");
    const auto rows = read_csv(priv::test_games_path);
    if (rows.empty())
    {
        println("CSV move conversion test: no rows read from [{}]", priv::test_games_path.string());
        return;
    }

    int total = 0;
    int success = 0;
    int uci_failures = 0;
    int fen_failures = 0;

    for (const auto &row : rows)
    {
        if (row.size() < 5)
        {
            LOG("Skipping malformed row (expected 5 columns, got {})", row.size());
            continue;
        }

        const string &game_id = row[0];
        const string &ply = row[1];
        const string &fen_before = row[2];
        const string &uci = row[3];
        const string &fen_after = row[4];

        total++;
        bool b_success = true;

        BitBoard this_board = BitBoard::start_position();
        Move mov;
        string uci_converted;
        try
        {
            this_board = BitBoard(fen_before);
            mov = Engine::uci_to_move(uci, this_board);
            uci_converted = Engine::move_to_uci(mov, this_board);
        }
        catch (std::exception &e)
        {
            println("Exception. Fen: [{}]\n uci: [{}]\n mov type: [{}]\n", fen_before, uci,
                    static_cast<int>(mov.type));
            continue;
        }
        if (uci != uci_converted)
        {
            println("[game {} ply {}] Failed to encode and unencode uci: [{}] -> [{}]", game_id,
                    ply, uci, uci_converted);
            b_success = false;
            uci_failures++;
        }

        this_board.apply_move(mov);
        const string fen_converted = this_board.to_fen();
        if (fen_converted != fen_after)
        {
            println(
                "[game {} ply {}] Failed to apply move [{}] from [{}]:\n\tCorrect: [{}]\n\t  "
                "Wrong: [{}]",
                game_id, ply, uci, fen_before, fen_after, fen_converted);
            b_success = false;
            fen_failures++;
        }

        success += b_success ? 1 : 0;
    }

    println(
        "Move conversion test complete\nPass rate: {:.0f}%\nuci failures: {}, fen failures: "
        "{}\n",
        static_cast<float>(success) / static_cast<float>(total) * 100, uci_failures, fen_failures);
}

void test_zobrist_hash()
{
    println("Running Zobrist hashing test...");

    const auto rows = read_csv(priv::test_games_path);
    if (rows.empty())
    {
        println("Zobrist hashing test: no rows read from [{}]", priv::test_games_path.string());
        return;
    }

    int total = 0;
    int success = 0;
    int construction_failures = 0;
    int hash_failures = 0;

    for (const auto &row : rows)
    {
        if (row.size() < 5)
        {
            LOG("Skipping malformed row (expected 5 columns, got {})", row.size());
            continue;
        }

        const string &game_id = row[0];
        const string &ply = row[1];
        const string &fen_before = row[2];
        const string &uci = row[3];
        const string &fen_after = row[4];

        total++;
        bool b_success = true;

        try
        {
            // Construct board and hash from the position before the move.
            auto board = BitBoard(fen_before);
            ZobristHash hash(board);

            const uint64_t hash_before = hash.get();

            // Construct a fresh hash from the same board.
            ZobristHash reference_before(board);

            if (hash_before != reference_before.get())
            {
                println(
                    "[game {} ply {}] Hash construction is inconsistent:\n"
                    "\tFEN:      [{}]\n"
                    "\tHash:     {:016X}\n"
                    "\tReference:{:016X}",
                    game_id, ply, fen_before, hash_before, reference_before.get());

                b_success = false;
                construction_failures++;
            }

            // Convert UCI to a move and apply it.
            const Move mov = Engine::uci_to_move(uci, board);

            board.apply_move(mov);

            // Incrementally update the hash.
            hash.push(mov);

            const uint64_t incremental_hash = hash.get();

            // Construct the same hash from scratch from the resulting board.
            const ZobristHash reference_after(board);
            const uint64_t reference_hash = reference_after.get();

            if (incremental_hash != reference_hash)
            {
                println(
                    "[game {} ply {}] Zobrist hash mismatch after move [{}]:\n"
                    "\tBefore FEN: [{}]\n"
                    "\tAfter FEN:  [{}]\n"
                    "\tIncremental: {:016X}\n"
                    "\tReference:   {:016X}",
                    game_id, ply, uci, fen_before, fen_after, incremental_hash, reference_hash);

                b_success = false;
                hash_failures++;
            }
        }
        catch (const std::exception &e)
        {
            println(
                "[game {} ply {}] Exception testing Zobrist hash:\n"
                "\tFEN: [{}]\n"
                "\tUCI: [{}]\n"
                "\t{}",
                game_id, ply, fen_before, uci, e.what());

            b_success = false;
            construction_failures++;
        }

        success += b_success ? 1 : 0;
    }

    println(
        "Zobrist hashing test complete\n"
        "Pass rate: {:.0f}%\n"
        "construction failures: {}\n"
        "hash failures: {}\n",
        static_cast<float>(success) / static_cast<float>(total) * 100, construction_failures,
        hash_failures);
}

void test_board_history()
{
    println("Running board history test...");

    BoardHistory history;

    const ZobristHash hash_a{0x123456789ABCDEF0ULL};
    const ZobristHash hash_b{0xFEDCBA9876543210ULL};
    const ZobristHash hash_c{0x1111222233334444ULL};

    int total = 0;
    int success = 0;

    auto check = [&](bool condition, const string &description) -> void
    {
        total++;

        if (condition)
        {
            success++;
            return;
        }

        println("  FAILED: {}", description);
    };

    check(!history.is_threefold(hash_a), "Empty history should not contain a threefold repetition");

    history.push_back(hash_a);

    check(!history.is_threefold(hash_a),
          "One prior occurrence should not be a threefold repetition");

    history.push_back(hash_b);
    history.push_back(hash_a);

    check(history.is_threefold(hash_a), "Two prior occurrences should be a threefold repetition");
    check(!history.is_threefold(ZobristHash{}), "A different hash should not produce a repetition");

    history.pop_back();  // Remove third A

    check(!history.is_threefold(hash_a),
          "Popping the latest occurrence should remove the repetition");

    history.push_back(hash_a);

    check(history.is_threefold(hash_a),
          "Adding the second prior occurrence again should restore repetition");

    history.push_irreversible(hash_b);

    check(!history.is_threefold(hash_a),
          "Positions before an irreversible move should be discarded");

    check(!history.is_threefold(hash_b),
          "A single position after an irreversible move is not a repetition");

    history.push_back(hash_c);
    history.push_back(hash_b);
    history.push_back(hash_c);
    history.push_back(hash_b);

    check(history.is_threefold(hash_b),
          "Three occurrences after an irreversible move should be detected");

    check(!history.is_threefold(hash_a),
          "Old positions should remain excluded after irreversible move");

    println(
        "Board history test complete\n"
        "Pass rate: {:.0f}% ({}/{})\n",
        static_cast<float>(success) / static_cast<float>(total) * 100.0F, success, total);
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
