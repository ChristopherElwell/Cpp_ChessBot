#include "eval.h"

#include <bit>

#include "bitboard.h"
#include "bitscan.h"
#include "data.h"

using namespace std;

namespace
{
constexpr int mg_eg_piece_threshold = 24;

template <piece_t Piece>
void evaluate_pc(const BitBoard& board, int& mg_eval, int& eg_eval, int& mg_to_eg_eval)
{
    for (auto pc_bit : bit_scan(board[Piece]))
    {
        mg_eval += pc_sq_table::midgame<Piece>[countr_zero(pc_bit)];
        eg_eval += pc_sq_table::endgame<Piece>[countr_zero(pc_bit)];
        mg_to_eg_eval += pc_sq_table::mid_to_endgame_pc_val<Piece>;
    }
}
}  // namespace

template <side_t Side>
auto evaluate(const BitBoard& board) -> int
{
    int white_mg_eval = 0;
    int white_eg_eval = 0;
    int black_mg_eval = 0;
    int black_eg_eval = 0;
    int mg_to_eg_counter = 0;

    evaluate_pc<piece_t::white_pawn>(board, white_mg_eval, white_eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::white_knight>(board, white_mg_eval, white_eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::white_bishop>(board, white_mg_eval, white_eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::white_rook>(board, white_mg_eval, white_eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::white_queen>(board, white_mg_eval, white_eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::white_king>(board, white_mg_eval, white_eg_eval, mg_to_eg_counter);

    evaluate_pc<piece_t::black_pawn>(board, black_mg_eval, black_eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::black_knight>(board, black_mg_eval, black_eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::black_bishop>(board, black_mg_eval, black_eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::black_rook>(board, black_mg_eval, black_eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::black_queen>(board, black_mg_eval, black_eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::black_king>(board, black_mg_eval, black_eg_eval, mg_to_eg_counter);

    int mg_eval = 0;
    int eg_eval = 0;
    if constexpr (Side == side_t::white)
    {
        mg_eval = white_mg_eval - black_mg_eval;
        eg_eval = white_eg_eval - black_eg_eval;
    }
    else
    {
        mg_eval = black_mg_eval - white_mg_eval;
        eg_eval = black_eg_eval - white_eg_eval;
    }

    if (mg_to_eg_counter > mg_eg_piece_threshold)
    {
        return mg_eval;
    }
    return ((mg_eval * mg_to_eg_counter) + (eg_eval * (mg_eg_piece_threshold - mg_to_eg_counter))) /
           mg_eg_piece_threshold;
}

template auto evaluate<side_t::white>(const BitBoard& board) -> int;
template auto evaluate<side_t::black>(const BitBoard& board) -> int;
