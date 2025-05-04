#include "control.h"
#include "search.h"
#include "helpers.h"
#include "move.h"
#include "move_gen.h"
#include <string>

namespace control
{
    std::string get_bot_move_at_depth(std::string FEN, int max_depth)
    {
        BitBoard bb = BitBoard(FEN);
        MoveGen mg;
        SearchRes *result = search(bb, mg, max_depth, INT16_MIN, INT16_MAX);
        helpers::print_principal_variation(result, bb);
        std::string best = helpers::move_to_uci(result->best_move, bb);
        helpers::free_search_result(result);
        return best;
    }

    void receiver()
    {
        std::string line;
        while (std::getline(std::cin, line))
        {
            std::string move = get_bot_move_at_depth(line, 5);
            std::cerr << move << std::endl; // Output response
            std::cerr.flush();              // Ensure Python gets it immediately
        }
    }
}