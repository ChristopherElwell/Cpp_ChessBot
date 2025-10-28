#include "eval.h"

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
    for (auto pc_bit : BitScan(board[Piece]))
    {
        mg_eval += pc_sq_table::midgame<Piece>[__builtin_ctzll(pc_bit)];
        eg_eval += pc_sq_table::endgame<Piece>[__builtin_ctzll(pc_bit)];
        mg_to_eg_eval += pc_sq_table::mid_to_endgame_pc_val<Piece>;
    }
}
}  // namespace

auto evaluate(const BitBoard& board) -> int
{
    int mg_eval = 0;
    int eg_eval = 0;
    int mg_to_eg_counter = 0;

    evaluate_pc<piece_t::black_pawn>(board, mg_eval, eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::black_knight>(board, mg_eval, eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::black_bishop>(board, mg_eval, eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::black_rook>(board, mg_eval, eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::black_queen>(board, mg_eval, eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::black_king>(board, mg_eval, eg_eval, mg_to_eg_counter);

    mg_eval = -mg_eval;
    eg_eval = -eg_eval;

    evaluate_pc<piece_t::white_pawn>(board, mg_eval, eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::white_knight>(board, mg_eval, eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::white_bishop>(board, mg_eval, eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::white_rook>(board, mg_eval, eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::white_queen>(board, mg_eval, eg_eval, mg_to_eg_counter);
    evaluate_pc<piece_t::white_king>(board, mg_eval, eg_eval, mg_to_eg_counter);

    if (mg_to_eg_counter > mg_eg_piece_threshold)
    {
        return mg_eval;
    }
    return (mg_eval * mg_to_eg_counter + eg_eval * (mg_eg_piece_threshold - mg_to_eg_counter)) /
           mg_eg_piece_threshold;
}