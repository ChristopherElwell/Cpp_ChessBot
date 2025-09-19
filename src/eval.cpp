#include "eval.h"
#include "bitboard.h"
#include "data.h"
#include "bitscan.h"

using namespace std;

auto evaluate(const BitBoard &board) -> int
{
    int mg_eval = 0;
    int eg_eval = 0;
    int mg_to_eg_counter = 0;
    int pos = 0;

    for (auto piece : PieceRange::White())
    {
        for (auto pc_bit : BitScan(board[piece]))
        {
            mg_to_eg_counter += data::MG_EG_PCVALS[static_cast<int>(piece)];
            pos = __builtin_ctzll(pc_bit);
            mg_eval += data::MG_PCTBL[pos + (static_cast<int>(piece) << 6)];
            eg_eval += data::EG_PCTBL[pos + (static_cast<int>(piece) << 6)];
        }
    }
    for (auto piece : PieceRange::Black())
    {
        for (auto pc_bit : BitScan(board[piece]))
        {
            mg_to_eg_counter += data::MG_EG_PCVALS[static_cast<int>(piece)];
            pos = __builtin_ctzll(pc_bit);
            mg_eval -= data::MG_PCTBL[pos + (static_cast<int>(piece) << 6)];
            eg_eval -= data::EG_PCTBL[pos + (static_cast<int>(piece) << 6)];
        }
    }
    static const int max_piece_val = 24;
    if (mg_to_eg_counter > max_piece_val)
    {
        return mg_eval;
    }
    return (mg_eval * mg_to_eg_counter + eg_eval * (max_piece_val - mg_to_eg_counter)) / max_piece_val;
}