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
        else if (arg == "--help" || arg == "-h")
        {
            std::cout << "Usage:\n"
                      << "  chess                            Run UCI mode\n"
                      << "  chess --uci                      Run UCI mode\n"
                      << "  chess --puzzles N                Run N puzzles\n"
                      << "  chess --perft N                  Run perft to depth N\n"
                      << "  chess --conversion               Run the move conversion test\n"
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
