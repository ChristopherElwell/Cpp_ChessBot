#pragma once
#include "bitboard.h"
#include "move_gen.h"
#include <iostream>
#include <cstdint>

void print_bitboard(const uint64_t& b);
uint64_t sq_from_name(char file, char rank);
std::ostream& operator<<(std::ostream& os, Piece p);
std::ostream& operator<<(std::ostream& os, movType m);
std::ostream& operator<<(std::ostream& os, const Move& m);
std::ostream& operator<<(std::ostream& os, const BitBoard& bb);
void print_move_short(const Move& m);
std::string move_to_uci(const Move& mov, const BitBoard& board);