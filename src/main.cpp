#include "helpers.h"
#include "move_gen.h"
#include "move.h"
#include "bitboard.h"
#include "data.h"
#include "testing.h"
#include "search.h"
#include "eval.h"
#include "control.h"
#include <string>
#include <cstdint>
#include <iostream>
#include <array>

int main(){
    std::string move = control::get_bot_move("8/3kr1P1/pR1B3p/2Pb4/8/P4qP1/7P/4K3 w - - 0 5",5);
}