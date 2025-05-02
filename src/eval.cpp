#include "eval.h"
#include "bitboard.h"
#include "helpers.h"
#include <cstdint>

int16_t evaluate(const BitBoard& bb){
    int16_t mg_eval = 0;
    int16_t eg_eval = 0;
    int16_t mg_to_eg_counter = 0;
    int pos;

    for (Piece pc : PieceRange::White()){
        for (uint64_t pc_bit : helpers::BitScan(bb[pc])){
            mg_to_eg_counter += data::MG_EG_PCVALS[static_cast<int>(pc)];
            pos = __builtin_ctzll(pc_bit);
            mg_eval += data::MG_PCTBL[pos+(static_cast<int>(pc)<<6)];
            eg_eval += data::EG_PCTBL[pos+(static_cast<int>(pc)<<6)];
        }
    }
    for (Piece pc : PieceRange::Black()){
        for (uint64_t pc_bit : helpers::BitScan(bb[pc])){
            mg_to_eg_counter += data::MG_EG_PCVALS[static_cast<int>(pc)];
            pos = __builtin_ctzll(pc_bit);
            mg_eval -= data::MG_PCTBL[pos+(static_cast<int>(pc)<<6)];
            eg_eval -= data::EG_PCTBL[pos+(static_cast<int>(pc)<<6)];
        }
    }
    if (mg_to_eg_counter > 24){
        return mg_eval;
    }
    return (mg_eval * mg_to_eg_counter + eg_eval * (24 - mg_to_eg_counter)) / 24;
}