#include "control.h"
#include "search.h"
#include "helpers.h"
#include "bitboard.h"
#include <string>
#include <iostream>
#include <chrono>
#include <climits>

using namespace std;
// NOLINTBEGIN(misc-use-internal-linkage)
namespace control
{

    auto get_bot_move(const string &FEN, double time_s) -> string
    {
        auto board = BitBoard(FEN);
        auto start = std::chrono::steady_clock::now();
        int depth = 1;
        SearchRes *result = nullptr;
        cout << "\n";
        while (chrono::duration<double>(chrono::steady_clock::now() - start).count() <= time_s)
        {
            cout << depth << ", ";
            result = search(board, depth++, INT_MIN, INT_MAX);
        }
        cout << "\n";
        helpers::print_principal_variation(result, board);
        string best = helpers::move_to_uci(result->best_move, board);
        helpers::free_search_result(result);
        return best;
    }

    auto get_bot_move_at_depth_uci(const string &FEN, int max_depth) -> string
    {
        auto board = BitBoard(FEN);
        SearchRes *result = search(board, max_depth, INT_MIN, INT_MAX);
        helpers::print_principal_variation(result, board);
        string best = helpers::move_to_uci(result->best_move, board); // NOLINT(misc-const-correctness)
        helpers::free_search_result(result);
        return best;
    }

    auto get_bot_move_at_depth_algebreic(const string &FEN, int max_depth) -> string
    {
        auto board = BitBoard(FEN);
        SearchRes *result = search(board, max_depth, INT_MIN, INT_MAX);
        helpers::print_principal_variation(result, board);
        string best = helpers::move_to_algebreic(result->best_move, board); // NOLINT(misc-const-correctness)
        helpers::free_search_result(result);
        return best;
    }

    void receiver()
    {
        string line;
        while (getline(cin, line))
        {
            const string move = get_bot_move(line, 1);
            cerr << move << "\n";
            cerr.flush();
        }
    }

}
// NOLINTEND(misc-use-internal-linkage)
