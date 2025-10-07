#include "helpers.h"
#include "bitboard.h"
#include "data.h"
#include "move.h"
#include "move_gen.h"
#include "search.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <print>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace helpers
{
void free_search_result(SearchRes *result)
{
    if (result == nullptr)
    {
        return;
    }

    SearchRes *child = result->best_result;
    result->best_result = nullptr;

    free_search_result(child);
    delete result;
}

auto sq_from_name(char file, char rank) -> uint64_t
{
    return masks::RANKS[rank - '1'] & masks::FILES['h' - file];
}

void print_principal_variation(const SearchRes *searchres, BitBoard board)
{
    const SearchRes *curr = searchres;

    print("PV (eval = {}):", curr->best_eval);

    while (curr != nullptr && curr->best_result != nullptr)
    {
        print("{}, ", helpers::move_to_uci(curr->best_move, board));
        board.apply_move(curr->best_move);
        curr = curr->best_result;
    }
}

auto move_to_uci(const Move &mov, const BitBoard &board) -> string
{
    string out;
    uint64_t starting_sq = 0;
    uint64_t ending_sq = 0;
    switch (mov.type)
    {
    case movType::QUIET:
    case movType::CAPTURE:
    case movType::CASTLE_KINGSIDE:
    case movType::CASTLE_QUEENSIDE:
        starting_sq = mov.mov1 & board[mov.pc1];
        ending_sq = mov.mov1 & ~board[mov.pc1];
        out += data::SQUARES[__builtin_ctzll(starting_sq)];
        out += data::SQUARES[__builtin_ctzll(ending_sq)];
        return out;
    case movType::PROMOTE:
        starting_sq = mov.mov1 & board[mov.pc1];
        ending_sq = mov.mov2;
        out += data::SQUARES[__builtin_ctzll(starting_sq)];
        out += data::SQUARES[__builtin_ctzll(ending_sq)];
        out += data::PIECE_CODES[static_cast<size_t>(mov.pc2)];
        return out;
    case movType::CAPTURE_PROMOTE:
        starting_sq = mov.mov1 & board[mov.pc1];
        ending_sq = mov.mov3;
        out += data::SQUARES[__builtin_ctzll(starting_sq)];
        out += data::SQUARES[__builtin_ctzll(ending_sq)];
        out += data::PIECE_CODES[static_cast<size_t>(mov.pc3)];
        return out;
    case movType::BOOK_END:
        return "BOOK END";
    default:
        return "UNKNOWN";
    }

    return out;
}

auto move_to_algebreic(const Move &mov, BitBoard board) -> string
{
    const uint64_t to_pos = mov.mov1 & ~board[mov.pc1];
    const uint64_t from_pos = mov.mov1 & ~to_pos;
    string out;
    string destination = data::SQUARES[__builtin_ctzll(to_pos)];
    string origin = data::SQUARES[__builtin_ctzll(from_pos)];
    if (mov.pc1 == Piece::WHITE_PAWN || mov.pc1 == Piece::BLACK_PAWN)
    {
        switch (mov.type)
        {
        case movType::QUIET:
            out = destination;
            break;
        case movType::CAPTURE:
            out = format("{}x{}", origin[0], destination);
            break;
        case movType::PROMOTE:
            out = format("{}{}", destination, data::PIECE_CODES[static_cast<int>(mov.pc2) % 6]);
            break;
        case movType::CAPTURE_PROMOTE:
            out = format("{}{}", origin[0], data::SQUARES[__builtin_ctzll(mov.mov2)],
                         data::PIECE_CODES[static_cast<int>(mov.pc3) % 6]);
            break;
        case movType::CASTLE_KINGSIDE:
        case movType::CASTLE_QUEENSIDE:
        default:
            return "Unknown";
            break;
        }
    }
    else
    {
        switch (mov.type)
        {
        case movType::QUIET:
            out = string(1, data::PIECE_CODES[static_cast<int>(mov.pc1) % 6]) + destination;
            break;
        case movType::CAPTURE:

            out = string(1, data::PIECE_CODES[static_cast<int>(mov.pc1) % 6]) + "x" + destination;
            break;
        case movType::CASTLE_KINGSIDE:
            out = "O-O";
            break;
        case movType::CASTLE_QUEENSIDE:
            out = "O-O-O";
            break;
        case movType::CAPTURE_PROMOTE:
        default:
            return "Unknown";
        }
    }
    auto move_gen = MoveGen(board);
    if (board.whites_turn())
    {
        board.apply_move(mov);
        return ((board[Piece::BLACK_KING] & move_gen.get_white_attackers(board)) != 0) ? out + "+"
                                                                                       : out;
    }

    board.apply_move(mov);
    return ((board[Piece::WHITE_KING] & move_gen.get_black_attackers(board)) != 0) ? out + "+"
                                                                                   : out;
}
} // namespace helpers
