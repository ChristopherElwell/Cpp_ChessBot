#include <cstdlib>
#include <exception>
#include <iostream>
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

constexpr auto is_diagnostic(mode_t mode) -> bool;

auto run_task(const task_t& task) -> void;

}  // namespace

// NOLINTNEXTLINE(bugprone-exception-escape)
auto main(int argc, char* argv[]) -> int
{
    try
    {
        const vector<task_t> args = parse_arguments(argc, span(argv, static_cast<size_t>(argc)));

        if (args.empty())
        {
            Engine engine;
            engine.uci_loop();
            return 0;
        }

        for (const task_t& task : args)
        {
            run_task(task);
        }
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "Fatal error: unknown exception\n";
        return 1;
    }
}

namespace
{
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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
        else if (arg == "--divide")
        {
            if (i + 2 >= argc)
            {
                std::cerr << "--divide requires a FEN and a depth, e.g.\n"
                          << "  chess --divide \"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w "
                             "KQkq - 0 1\" 3\n";
                std::exit(EXIT_FAILURE);
            }
            task_t task{};
            task.mode = mode_t::divide;
            task.fen = argv[++i];
            task.count = std::stoi(argv[++i]);
            args.push_back(task);
        }
        else if (arg == "--checkmove")
        {
            if (i + 2 >= argc)
            {
                std::cerr << "--checkmove requires a FEN and a UCI move, e.g.\n"
                          << "  chess --checkmove \"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/"
                             "PPPBBPPP/R3K2R w KQkq -\" d5e6\n";
                std::exit(EXIT_FAILURE);
            }
            task_t task{};
            task.mode = mode_t::checkmove;
            task.fen = argv[++i];
            task.uci = argv[++i];
            args.push_back(task);
        }
        else if (arg == "--verify")
        {
            if (i + 2 >= argc)
            {
                std::cerr << "--verify requires a FEN and a depth, e.g.\n"
                          << "  chess --verify \"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w "
                             "KQkq - 0 1\" 3\n";
                std::exit(EXIT_FAILURE);
            }
            task_t task{};
            task.mode = mode_t::verify;
            task.fen = argv[++i];
            task.count = std::stoi(argv[++i]);
            args.push_back(task);
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
                << "  chess --conversion --perft 5\n"
                << "\n"
                << "--divide, --checkmove and --verify are one-off diagnostics and must be\n"
                << "used on their own, not combined with other flags.\n";
            std::exit(EXIT_SUCCESS);
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << '\n';
            std::exit(EXIT_FAILURE);
        }
    }

    if (args.size() > 1)
    {
        for (const task_t& task : args)
        {
            if (is_diagnostic(task.mode))
            {
                std::cerr << "--divide, --checkmove and --verify must be used on their own, "
                             "not combined with other flags\n";
                std::exit(EXIT_FAILURE);
            }
        }
    }

    return args;
}

constexpr auto is_diagnostic(mode_t mode) -> bool
{
    return mode == mode_t::divide || mode == mode_t::checkmove || mode == mode_t::verify;
}

auto run_task(const task_t& task) -> void
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
        case mode_t::divide:
            run_perft_divide(task.fen, task.count);
            break;
        case mode_t::checkmove:
            debug_check_state_after_move(task.fen, task.uci);
            break;
        case mode_t::verify:
            run_perft_verify(task.fen, task.count);
            break;
    }
}
}  // namespace
