#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "engine.h"
#include "testing.h"

using namespace std;

namespace
{
auto parse_arguments(int argc, span<char* const> argv) -> vector<task_t>;
}

auto main(int argc, char* argv[]) -> int
{
    // Handled separately from the rest of the flags: it takes a FEN + depth pair
    // rather than a single count, and is a one-off diagnostic rather than something
    // meant to be combined with other tasks in one invocation.
    if (argc >= 2 && std::string_view(argv[1]) == "--divide")
    {
        if (argc < 4)
        {
            std::cerr << "--divide requires a FEN and a depth, e.g.\n"
                      << "  chess --divide \"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - "
                         "0 1\" 3\n";
            return EXIT_FAILURE;
        }
        const std::string fen = argv[2];
        const int depth = std::stoi(argv[3]);
        run_perft_divide(fen, depth);
        return 0;
    }
    if (argc >= 2 && std::string_view(argv[1]) == "--checkmove")
    {
        if (argc < 4)
        {
            std::cerr << "--checkmove requires a FEN and a UCI move, e.g.\n"
                      << "  chess --checkmove \"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/"
                         "R3K2R w KQkq -\" d5e6\n";
            return EXIT_FAILURE;
        }
        const std::string fen = argv[2];
        const std::string uci = argv[3];
        debug_check_state_after_move(fen, uci);
        return 0;
    }
    if (argc >= 2 && std::string_view(argv[1]) == "--verify")
    {
        if (argc < 4)
        {
            std::cerr << "--verify requires a FEN and a depth, e.g.\n"
                      << "  chess --verify \"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - "
                         "0 1\" 3\n";
            return EXIT_FAILURE;
        }
        const std::string fen = argv[2];
        const int depth = std::stoi(argv[3]);
        run_perft_verify(fen, depth);
        return 0;
    }

    const vector<task_t> args = parse_arguments(argc, span(argv, static_cast<size_t>(argc)));

    if (args.empty())
    {
        // No flags given: default to UCI mode, same as before.
        Engine engine;
        engine.uci_loop();
        return 0;
    }

    for (const task_t& task : args)
    {
        switch (task.mode)
        {
            case mode_t::uci:
            {
                Engine engine;
                engine.uci_loop();
                break;
            }
            case mode_t::puzzles:
                test_puzzles(task.count);
                break;
            case mode_t::perft:
                run_perft_test(task.count);
                break;
            case mode_t::conversion:
                test_move_conversion();
                break;
            case mode_t::ttable:
                test_ttable();
                break;
            case mode_t::history:
                test_board_history();
                break;
            case mode_t::zobrist:
                test_zobrist_hash();
                break;
        }
    }
    return 0;
}
namespace
{
auto parse_arguments(int argc, span<char* const> argv) -> vector<task_t>
{
    vector<task_t> args{};
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg = argv[i];
        if (arg == "--puzzles")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "--puzzles requires a number\n";
                std::exit(EXIT_FAILURE);
            }
            task_t task{};
            task.mode = mode_t::puzzles;
            task.count = std::stoi(argv[++i]);
            if (task.count < 0)
            {
                std::cerr << "--puzzles requires a non-negative number\n";
                std::exit(EXIT_FAILURE);
            }
            args.push_back(task);
        }
        else if (arg == "--perft")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "--perft requires a depth\n";
                std::exit(EXIT_FAILURE);
            }
            task_t task{};
            task.mode = mode_t::perft;
            task.count = std::stoi(argv[++i]);
            if (task.count < 0)
            {
                std::cerr << "--perft requires a non-negative depth\n";
                std::exit(EXIT_FAILURE);
            }
            args.push_back(task);
        }
        else if (arg == "--conversion")
        {
            args.push_back(task_t{.mode = mode_t::conversion});
        }
        else if (arg == "--history")
        {
            args.push_back(task_t{.mode = mode_t::history});
        }
        else if (arg == "--zobrist")
        {
            args.push_back(task_t{.mode = mode_t::zobrist});
        }
        else if (arg == "--uci")
        {
            args.push_back(task_t{.mode = mode_t::uci});
        }
        else if (arg == "--ttable")
        {
            args.push_back(task_t{.mode = mode_t::ttable});
        }
        else if (arg == "--help" || arg == "-h")
        {
            std::cout
                << "Usage:\n"
                << "  chess                            Run UCI mode\n"
                << "  chess --uci                      Run UCI mode\n"
                << "  chess --puzzles N                Run N puzzles\n"
                << "  chess --perft N                  Run perft to depth N\n"
                << "  chess --conversion               Run the move conversion test\n"
                << "  chess --ttable                   Run the move tranposition table test\n"
                << "  chess --divide FEN DEPTH         Show per-move perft counts (\"divide\") at\n"
                << "                                    FEN/DEPTH, for debugging move gen. See\n"
                << "                                    perft_debug.py.\n"
                << "  chess --checkmove FEN UCI        Compare incremental vs fresh-parsed\n"
                << "                                    state after applying UCI to FEN, to\n"
                << "                                    catch apply_move state-sync bugs.\n"
                << "  chess --verify FEN DEPTH         Walk the perft tree from FEN to DEPTH,\n"
                << "                                    checking every apply/undo round trip\n"
                << "                                    restores the board exactly. Reports\n"
                << "                                    the exact move path to the first bad\n"
                << "                                    undo, if any.\n"
                << "  chess --help                     Show this help\n"
                << "\n"
                << "Flags can be combined to run multiple tests in one invocation, e.g.:\n"
                << "  chess --conversion --perft 5\n";
            std::exit(EXIT_SUCCESS);
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
    return args;
}
}  // namespace
