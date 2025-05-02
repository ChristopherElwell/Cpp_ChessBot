#include "move.h"
#include "move_gen.h"
#include "bitboard.h"
#include <cstdint>

Move::Move() = default;

Move::Move(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2, Piece pc3, uint64_t mov3, uint64_t info, movType type)
    : pc1(pc1), mov1(mov1), pc2(pc2), mov2(mov2), pc3(pc3), mov3(mov3), info(info), type(type) {}

Move Move::quiet(Piece pc1, uint64_t mov1, uint64_t info, uint64_t board_info){
    return Move(pc1,mov1,{},0ULL,{},0ULL,TURN_BIT | (board_info & ~masks::RANK_1 & ~masks::RANK_8) | info, movType::QUIET);
}

Move Move::capture(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2, uint64_t info, uint64_t board_info){
    return Move(pc1,mov1,pc2,mov2,{},0ULL,TURN_BIT | (board_info & ~masks::RANK_1 & ~masks::RANK_8) | info, movType::CAPTURE);
}

Move Move::promote(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2, uint64_t info, uint64_t board_info){
    return Move(pc1,mov1,pc2,mov2,{},0ULL,TURN_BIT | (board_info & ~masks::RANK_1 & ~masks::RANK_8) | info, movType::PROMOTE);
}

Move Move::promote_capture(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2, Piece pc3, uint64_t mov3, uint64_t info, uint64_t board_info){
    return Move(pc1,mov1,pc2,mov2,pc3,mov3,TURN_BIT | (board_info & ~masks::RANK_1 & ~masks::RANK_8) | info, movType::CAPTURE_PROMOTE);
}

Move Move::castle_kingside(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2, uint64_t info, uint64_t board_info){
    return Move(pc1,mov1,pc2,mov2,{},0ULL,TURN_BIT | (board_info & ~masks::RANK_1 & ~masks::RANK_8) | info, movType::CASTLE_KINGSIDE);
}

Move Move::castle_queenside(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2, uint64_t info, uint64_t board_info){
    return Move(pc1,mov1,pc2,mov2,{},0ULL,TURN_BIT | (board_info & ~masks::RANK_1 & ~masks::RANK_8) | info, movType::CASTLE_QUEENSIDE);
}

Move Move::copy(const Move& m){
    return Move(m.pc1,m.mov1,m.pc2,m.mov2,m.pc2,m.mov2,m.info,m.type);
}