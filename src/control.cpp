#include "control.h"
#include "search.h"
#include "helpers.h"
#include "move.h"
#include "move_gen.h"
#include <string>

namespace control {
    std::string get_bot_move(std::string FEN, int max_depth){
        BitBoard bb = BitBoard(FEN);
        MoveGen mg;
        SearchRes* result = search(bb,mg,max_depth,INT16_MIN,INT16_MAX);
        std::cout << result->best_move;
        helpers::print_principal_variation(result,bb);
        std::string best = helpers::move_to_uci(result->best_move,bb);
        helpers::free_search_result(result);
        return best;
    }
}