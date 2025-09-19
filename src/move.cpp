#include "move.h"
#include "bitboard.h"
#include "data.h"
#include <cstdint>

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
Move::Move(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2, Piece pc3,
           uint64_t mov3, uint64_t info, movType type)
    : pc1(pc1), mov1(mov1), pc2(pc2), mov2(mov2), pc3(pc3), mov3(mov3),
      info(info), type(type) {}

auto Move::quiet(Piece pc1, uint64_t mov1, uint64_t info, uint64_t board_info)
    -> Move {
  return Move{pc1,
              mov1,
              {},
              0ULL,
              {},
              0ULL,
              (board_info & ~masks::RANK_1 & ~masks::RANK_8) | info,
              movType::QUIET};
}

auto Move::capture(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2,
                   uint64_t info, uint64_t board_info) -> Move {
  return Move{pc1,
              mov1,
              pc2,
              mov2,
              {},
              0ULL,
              (board_info & ~masks::RANK_1 & ~masks::RANK_8) | info,
              movType::CAPTURE};
}

auto Move::promote(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2,
                   uint64_t info, uint64_t board_info) -> Move {
  return Move{pc1,
              mov1,
              pc2,
              mov2,
              {},
              0ULL,
              (board_info & ~masks::RANK_1 & ~masks::RANK_8) | info,
              movType::PROMOTE};
}

auto Move::promote_capture(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2,
                           Piece pc3, uint64_t mov3, uint64_t info,
                           uint64_t board_info) -> Move {
  return Move{pc1,
              mov1,
              pc2,
              mov2,
              pc3,
              mov3,
              (board_info & ~masks::RANK_1 & ~masks::RANK_8) | info,
              movType::CAPTURE_PROMOTE};
}

auto Move::castle_kingside(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2,
                           uint64_t info, uint64_t board_info) -> Move {
  return Move{pc1,
              mov1,
              pc2,
              mov2,
              {},
              0ULL,
              (board_info & ~masks::RANK_1 & ~masks::RANK_8) | info,
              movType::CASTLE_KINGSIDE};
}

auto Move::castle_queenside(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2,
                            uint64_t info, uint64_t board_info) -> Move {
  return Move{pc1,
              mov1,
              pc2,
              mov2,
              {},
              0ULL,
              (board_info & ~masks::RANK_1 & ~masks::RANK_8) | info,
              movType::CASTLE_QUEENSIDE};
}

Move::Move()
    : pc1(Piece::PIECECOUNT), mov1(0), pc2(Piece::PIECECOUNT), mov2(0),
      pc3(Piece::PIECECOUNT), mov3(0), info(0), type(movType::BOOK_END) {}

auto Move::copy(const Move &mov) -> Move {
  return Move{mov.pc1, mov.mov1, mov.pc2,  mov.mov2,
              mov.pc3, mov.mov3, mov.info, mov.type};
}
// NOLINTEND(bugprone-easily-swappable-parameters)
