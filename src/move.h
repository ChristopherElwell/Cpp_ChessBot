#include "move_gen.h"
#include "bitboard.h"
#include <cstdint>

struct Move {
    movType type;
    uint64_t mov1, mov2, mov3;
    Piece pc1, pc2, pc3;
    uint64_t info;

    Move(Piece pc1, uint64_t mov1,
                   Piece pc2, uint64_t mov2,
                   Piece pc3, uint64_t mov3,
                   uint64_t info, movType type);

    static Move quiet(Piece pc1, uint64_t mov1, uint64_t info, uint64_t board_info);
    static Move capture(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2, uint64_t info, uint64_t board_info);
    static Move promote(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2, uint64_t info, uint64_t board_info);
    static Move promote_capture(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2, Piece pc3, uint64_t mov3, uint64_t info, uint64_t board_info);
    static Move castle_kingside(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2, uint64_t info, uint64_t board_info);
    static Move castle_queenside(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2, uint64_t info, uint64_t board_info);
    static Move copy(const Move&);
};
