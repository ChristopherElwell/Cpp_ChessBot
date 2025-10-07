#pragma once
#include "bitboard.h"
#include "move.h"
#include "search.h"
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace helpers
{

void free_search_result(SearchRes *result);
void print_bitboard(const uint64_t &board);
auto sq_from_name(char file, char rank) -> uint64_t;
void print_move_verbose(const Move &move);
auto move_to_uci(const Move &mov, const BitBoard &board) -> std::string;
void print_principal_variation(const SearchRes *searchres, BitBoard board);
auto move_to_algebreic(const Move &mov, BitBoard board) -> std::string;
} // namespace helpers
