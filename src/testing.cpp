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
#include <stdexcept>
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
auto read_csv(const filesystem::path &filename, size_t lines = numeric_limits<size_t>::max())
    -> vector<vector<string>>;
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
        const inv_move inverse = board.apply_move<Side>(move);
        // check if move leaves white king in check
        if (move_gen.is_king_in_check<Side>())
        {
            board.undo_move<Side>(move, inverse);
            continue;
        }
        perft += perft_search<~Side>(board, iter - 1);
        board.undo_move<Side>(move, inverse);
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
                print("\tFailed, Draft: {} | Correct Perft: {} | This Perft: {}\n", idx + 1,
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

namespace
{
template <side_t Side>
void perft_divide_impl(BitBoard &board, int depth)
{
    auto move_gen = MoveGen(board);
    move_gen.gen<Side>();

    vector<pair<string, uint64_t>> results;
    uint64_t total = 0;

    for (const Move &move : move_gen)
    {
        const inv_move inverse = board.apply_move<Side>(move);
        if (move_gen.is_king_in_check<Side>())
        {
            board.undo_move<Side>(move, inverse);
            continue;
        }
        const uint64_t count = perft_search<~Side>(board, depth - 1);
        results.emplace_back(Engine::move_to_uci(move), count);
        total += count;
        board.undo_move<Side>(move, inverse);
    }

    for (const auto &[uci, count] : results)
    {
        println("{}: {}", uci, count);
    }
    println("\nNodes searched: {}", total);
}
}  // namespace

void run_perft_divide(const string &fen, int depth)
{
    if (depth < 1)
    {
        println("Depth must be >= 1");
        return;
    }
    auto board = BitBoard(fen);
    if (board.side_to_move() == side_t::white)
    {
        perft_divide_impl<side_t::white>(board, depth);
    }
    else
    {
        perft_divide_impl<side_t::black>(board, depth);
    }
}

void debug_check_state_after_move(const string &fen, const string &uci)
{
    auto board = BitBoard(fen);
    const Move mov = Engine::uci_to_move(uci, board);

    if (board.side_to_move() == side_t::white)
    {
        board.apply_move<side_t::white>(mov);
    }
    else
    {
        board.apply_move<side_t::black>(mov);
    }

    const uint64_t incremental_hash = board.hash().get();
    const string fen_after = board.to_fen();
    const ZobristHash fresh_hash = ZobristHash(BitBoard(fen_after));

    println("Move:                 {}", uci);
    println("FEN after (to_fen()): {}", fen_after);
    println("Incremental hash:     {:016X}", incremental_hash);
    println("Fresh-parse hash:     {:016X}", fresh_hash.get());
    if (incremental_hash == fresh_hash.get())
    {
        println(
            "MATCH -- hash-visible state (pieces, side, castling rights, ep square) is "
            "consistent.\nIf perft still disagrees here, the stale field is something NOT covered "
            "by the hash -- e.g. a combined occupancy/attack/checkers bitboard used only by move "
            "generation.");
    }
    else
    {
        println(
            "MISMATCH -- apply_move is leaving hash-relevant state out of sync (piece placement, "
            "side to move, castling rights, or en passant square).");
    }
}

namespace
{
auto join_path(const vector<string> &path) -> string
{
    if (path.empty())
    {
        return "(root)";
    }
    string out;
    for (const auto &uci : path)
    {
        if (!out.empty())
        {
            out += ' ';
        }
        out += uci;
    }
    return out;
}

template <side_t Side>
auto perft_verify_impl(BitBoard &board, int iter, vector<string> &path) -> uint64_t
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
        const uint64_t hash_before = board.hash().get();
        const inv_move inverse = board.apply_move<Side>(move);

        if (move_gen.is_king_in_check<Side>())
        {
            board.undo_move<Side>(move, inverse);
        }
        else
        {
            path.push_back(Engine::move_to_uci(move));
            perft += perft_verify_impl<~Side>(board, iter - 1, path);
            path.pop_back();
            board.undo_move<Side>(move, inverse);
        }

        const uint64_t hash_after_undo = board.hash().get();
        if (hash_after_undo != hash_before)
        {
            path.push_back(Engine::move_to_uci(move));
            println("UNDO MISMATCH\n  Move path to failure: {}", join_path(path));
            println("  Hash before apply:    {:016X}", hash_before);
            println("  Hash after undo:      {:016X}", hash_after_undo);
            println(
                "  -> undo_move did not perfectly reverse apply_move for this move type / "
                "position. This is state left behind for the *next* sibling move to inherit, "
                "not a move-generation bug.");
            throw runtime_error("perft_verify: undo did not restore state");
        }
    }
    return perft;
}
}  // namespace

void run_perft_verify(const string &fen, int depth)
{
    auto board = BitBoard(fen);
    vector<string> path;
    try
    {
        const uint64_t total = board.side_to_move() == side_t::white
                                   ? perft_verify_impl<side_t::white>(board, depth, path)
                                   : perft_verify_impl<side_t::black>(board, depth, path);
        println("No undo mismatch found through depth {}. Perft({}) = {}", depth, depth, total);
    }
    catch (const exception &e)
    {
        println("Stopped early: {}", e.what());
    }
}

void test_puzzles(size_t count)
{
    println("Running puzzle test. {} puzzles...", count);
    const vector<vector<string>> pzls = read_csv(priv::puzzles_path, count);
    size_t idx = 0;
    int passed = 0;
    Engine engine;
    count = min(count, pzls.size());
    chrono::milliseconds sum_time = {};
    for (auto pzl : pzls)
    {
        string &fen = pzl[1];
        const string &pv_str = pzl[2];
        istringstream iss(pv_str);
        string answer_uci;
        string first_move_uci;
        iss >> first_move_uci;
        iss >> answer_uci;

        if (idx++ >= count)
        {
            break;
        }

        const auto clock_start = chrono::steady_clock::now();

        BitBoard board = BitBoard::start_position();
        try
        {
            board = BitBoard{fen};
        }
        catch (invalid_argument &e)
        {
            println("{}", e.what());
            continue;
        }
        Engine engine;
        Move first_move = Engine::uci_to_move(first_move_uci, board);
        board.apply_move(first_move);
        fen = board.to_fen();

        engine.load(board);
        future<void> future = engine.run(chrono::seconds{1});
        future.get();
        const auto clock_end = chrono::high_resolution_clock::now();
        sum_time += chrono::duration_cast<chrono::milliseconds>(clock_end - clock_start);

        if (engine.get_uci() == answer_uci)
        {
            passed++;
        }
        else
        {
            LOG("\nFAILED \n{}\nBot Move: [{}] Correct Move: [{}]\nPV: {}\n", fen, engine.get_uci(),
                answer_uci, engine.get_pv());
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
            uci_converted = Engine::move_to_uci(mov);
        }
        catch (std::exception &e)
        {
            println("Exception. Fen: [{}]\n uci: [{}]\n mov type: [{}]\n", fen_before, uci,
                    static_cast<int>(mov.type()));
            continue;
        }
        if (uci != uci_converted)
        {
            println("[game {} ply {}] Failed to encode and unencode uci: [{}] -> [{}]", game_id,
                    ply, uci, uci_converted);
            b_success = false;
            uci_failures++;
        }
        else
        {
            print(".");
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

void test_ttable()
{
    println("Running TTable test...");

    TTable ttable;
    ttable.set_size(4);

    int total = 0;
    int success = 0;

    auto check = [&](bool condition, const string &name) -> void
    {
        total++;

        if (condition)
        {
            success++;
            return;
        }

        println("TTable test failed: {}", name);
    };

    const BitBoard board = BitBoard::start_position();
    const uint64_t key = board.hash().get();

    search_args args{
        .depth = 5,
        .ply = 0,
        .alpha = -100,
        .beta = 100,
    };

    // --------------------------------
    // 1. Empty table should miss
    // --------------------------------
    {
        const auto result = ttable.probe(key, args);

        check(result.node == nullptr, "empty table returns nullptr");
        check(result.result == tt_probe_result::miss, "empty table returns miss");
    }

    // --------------------------------
    // 2. Store an exact entry
    // --------------------------------
    {
        const Move move{};

        ttable.store(
            tt_node{
                .key = key,
                .best_move = move,
                .eval = 25,
                .depth = 5,
                .flag = {},
            },
            args.alpha, args.beta, 0);

        const auto result = ttable.probe(key, args);

        check(result.node != nullptr, "stored entry can be probed");
        check(result.result == tt_probe_result::eval, "exact entry returns eval");
        check(result.node->key == key, "stored key matches");
        check(result.node->eval == 25, "stored eval matches");
        check(result.node->depth == 5, "stored depth matches");
        check(result.node->flag == tt_node_flag::exact, "entry is marked exact");
    }

    // --------------------------------
    // 3. Shallower entry should provide
    //    move but not eval
    // --------------------------------
    {
        search_args deeper_args{
            .depth = 6,
            .ply = 0,
            .alpha = -100,
            .beta = 100,
        };

        const auto result = ttable.probe(key, deeper_args);

        check(result.node != nullptr, "shallower entry still returns node");
        check(result.result == tt_probe_result::move, "shallower entry returns move");
    }

    // --------------------------------
    // 4. Exact entry at sufficient depth
    // --------------------------------
    {
        search_args same_depth{
            .depth = 5,
            .ply = 0,
            .alpha = -100,
            .beta = 100,
        };

        const auto result = ttable.probe(key, same_depth);

        check(result.result == tt_probe_result::eval, "entry at required depth returns eval");
        check(result.node->eval == 25, "exact eval is preserved");
    }

    // --------------------------------
    // 5. Bound that does not cause
    //    a cutoff should only provide move
    // --------------------------------
    {
        ttable.store(
            tt_node{
                .key = key,
                .best_move = {},
                .eval = 50,
                .depth = 5,
            },
            0, 100, 0);

        const auto result = ttable.probe(key, args);

        // 0 < 50 < 100, therefore exact.
        check(result.result == tt_probe_result::eval,
              "score inside alpha-beta window returns eval");
        check(result.node->flag == tt_node_flag::exact, "score inside window is exact");
    }

    // --------------------------------
    // 6. Fail-high should be lower bound
    // --------------------------------
    {
        ttable.store(
            tt_node{
                .key = key,
                .best_move = {},
                .eval = 150,
                .depth = 5,
            },
            -100, 100, 0);

        const auto result = ttable.probe(key, args);

        check(result.result == tt_probe_result::eval, "fail-high entry returns eval");
        check(result.node->flag == tt_node_flag::lower_bound, "fail-high is lower bound");
    }

    // --------------------------------
    // 7. Fail-low should be upper bound
    // --------------------------------
    {
        ttable.store(
            tt_node{
                .key = key,
                .best_move = {},
                .eval = -150,
                .depth = 5,
            },
            -100, 100, 0);

        const auto result = ttable.probe(key, args);

        check(result.result == tt_probe_result::eval, "fail-low entry returns eval");
        check(result.node->flag == tt_node_flag::upper_bound, "fail-low is upper bound");
    }

    // --------------------------------
    // 8. Same bound, but different window
    //    should not necessarily give eval
    // --------------------------------
    {
        search_args no_cutoff{
            .depth = 5,
            .ply = 0,
            .alpha = -200,
            .beta = 200,
        };

        const auto result = ttable.probe(key, no_cutoff);

        check(result.node != nullptr, "bound entry still returns node");
        check(result.result == tt_probe_result::move, "bound that cannot cutoff returns move");
    }

    // --------------------------------
    // 9. Shallower store must not replace
    //    deeper entry
    // --------------------------------
    {
        ttable.store(
            tt_node{
                .key = key,
                .best_move = {},
                .eval = 999,
                .depth = 3,
            },
            -100, 100, 0);

        const auto result = ttable.probe(key, args);

        check(result.node->depth == 5, "shallower entry does not replace deeper entry");
        check(result.node->eval == -150, "deeper entry eval is preserved");
    }

    // --------------------------------
    // 10. Deeper store should replace
    // --------------------------------
    {
        ttable.store(
            tt_node{
                .key = key,
                .best_move = {},
                .eval = 75,
                .depth = 7,
            },
            -100, 100, 0);

        const auto result = ttable.probe(key, args);

        check(result.node->depth == 7, "deeper entry replaces old entry");
        check(result.node->eval == 75, "deeper entry eval is stored");
    }

    // --------------------------------
    // 11. clear() should invalidate entries
    // --------------------------------
    ttable.clear();

    {
        const auto result = ttable.probe(key, args);

        check(result.node == nullptr, "clear removes stored entry");
        check(result.result == tt_probe_result::miss, "clear causes probe miss");
    }

    println(
        "TTable test complete\nPass rate: {:.0f}%\n"
        "failures: {}\n",
        static_cast<float>(success) / static_cast<float>(total) * 100, total - success);
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

            const uint64_t incremental_hash = board.hash().get();

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
auto read_csv(const filesystem::path &filename, size_t lines) -> vector<vector<string>>
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
    size_t counter = 0;
    while (getline(file, line) && counter < lines)
    {
        vector<string> row;
        stringstream stream(line);
        string cell;

        while (getline(stream, cell, ','))
        {
            row.push_back(cell);
        }

        result.push_back(row);
        counter++;
    }

    return result;
}
}  // namespace
