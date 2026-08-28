#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string_view>

#include "engine.h"
#include "testing.h"

using namespace std;

namespace
{

auto parse_arguments(int argc, span<char* const> argv) -> arguments;

}
auto main(int argc, char* argv[]) -> int
{
    const arguments args = parse_arguments(argc, span(argv, static_cast<size_t>(argc)));

    switch (args.mode)
    {
        case mode::uci:
        {
            Engine engine;
            engine.uci_loop();
            break;
        }

        case mode::puzzles:
            test_puzzles(args.count);
            break;

        case mode::perft:
            run_perft_test(args.count);
            break;
    }

    return 0;
}

namespace
{
auto parse_arguments(int argc, span<char* const> argv) -> arguments
{
    arguments args{};

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

            args.mode = mode::puzzles;
            args.count = std::stoi(argv[++i]);
        }
        else if (arg == "--perft")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "--perft requires a depth\n";
                std::exit(EXIT_FAILURE);
            }

            args.mode = mode::perft;
            args.count = std::stoi(argv[++i]);
        }
        else if (arg == "--help" || arg == "-h")
        {
            std::cout << "Usage:\n"
                      << "  chess                  Run UCI mode\n"
                      << "  chess --puzzles N      Run N puzzles\n"
                      << "  chess --perft N        Run perft to depth N\n"
                      << "  chess --help           Show this help\n";

            std::exit(EXIT_SUCCESS);
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << '\n';
            std::exit(EXIT_FAILURE);
        }
    }

    if (args.count < 0)
    {
        std::cerr << "Argument must be non-negative\n";
        std::exit(EXIT_FAILURE);
    }

    return args;
}
}  // namespace
