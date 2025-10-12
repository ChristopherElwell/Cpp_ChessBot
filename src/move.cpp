#include "move.h"

#include <cstdint>
#include <format>

#include "bitboard.h"
#include "data.h"

using namespace std;

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
Move::Move(piece_t pc1, uint64_t mov1, piece_t pc2, uint64_t mov2, piece_t pc3, uint64_t mov3,
           uint64_t info, movType type)
    : pc1(pc1), mov1(mov1), pc2(pc2), mov2(mov2), pc3(pc3), mov3(mov3), info(info), type(type)
{
}

auto Move::quiet(piece_t pc1, uint64_t mov1, uint64_t info, uint64_t board_info) -> Move
{
    return Move{pc1,
                mov1,
                {},
                0ULL,
                {},
                0ULL,
                (board_info & ~masks::rank_1 & ~masks::rank_8) | info,
                movType::QUIET};
}

auto Move::capture(piece_t pc1, uint64_t mov1, piece_t pc2, uint64_t mov2, uint64_t info,
                   uint64_t board_info) -> Move
{
    return Move{pc1,
                mov1,
                pc2,
                mov2,
                {},
                0ULL,
                (board_info & ~masks::rank_1 & ~masks::rank_8) | info,
                movType::CAPTURE};
}

auto Move::promote(piece_t pc1, uint64_t mov1, piece_t pc2, uint64_t mov2, uint64_t info,
                   uint64_t board_info) -> Move
{
    return Move{pc1,
                mov1,
                pc2,
                mov2,
                {},
                0ULL,
                (board_info & ~masks::rank_1 & ~masks::rank_8) | info,
                movType::PROMOTE};
}

auto Move::promote_capture(piece_t pc1, uint64_t mov1, piece_t pc2, uint64_t mov2, piece_t pc3,
                           uint64_t mov3, uint64_t info, uint64_t board_info) -> Move
{
    return Move{pc1,
                mov1,
                pc2,
                mov2,
                pc3,
                mov3,
                (board_info & ~masks::rank_1 & ~masks::rank_8) | info,
                movType::CAPTURE_PROMOTE};
}

auto Move::castle_kingside(piece_t pc1, uint64_t mov1, piece_t pc2, uint64_t mov2, uint64_t info,
                           uint64_t board_info) -> Move
{
    return Move{pc1,
                mov1,
                pc2,
                mov2,
                {},
                0ULL,
                (board_info & ~masks::rank_1 & ~masks::rank_8) | info,
                movType::CASTLE_kingside};
}

auto Move::castle_queenside(piece_t pc1, uint64_t mov1, piece_t pc2, uint64_t mov2, uint64_t info,
                            uint64_t board_info) -> Move
{
    return Move{pc1,
                mov1,
                pc2,
                mov2,
                {},
                0ULL,
                (board_info & ~masks::rank_1 & ~masks::rank_8) | info,
                movType::CASTLE_queenside};
}

Move::Move()
    : pc1(piece_t::piece_count),
      mov1(0),
      pc2(piece_t::piece_count),
      mov2(0),
      pc3(piece_t::piece_count),
      mov3(0),
      info(0),
      type(movType::BOOK_END)
{
}

auto Move::copy(const Move &mov) -> Move
{
    return Move{mov.pc1, mov.mov1, mov.pc2, mov.mov2, mov.pc3, mov.mov3, mov.info, mov.type};
}

auto Move::to_string() const -> string
{
    string out = format("Move Type: {} | Primary piece_t: {}", move_type_to_string(type),
                        full_piece_names.at(static_cast<int>(pc1)));

    switch (type)
    {
        case movType::QUIET:
        case movType::CASTLE_kingside:
        case movType::CASTLE_queenside:
        case movType::BOOK_END:
            return out;
        case movType::CAPTURE:
            return format("{} | Captured piece_t: {}", out,
                          full_piece_names.at(static_cast<int>(pc2)));
        case movType::PROMOTE:
            return format("{} | Promotee piece_t: {}", out,
                          full_piece_names.at(static_cast<int>(pc2)));
        case movType::CAPTURE_PROMOTE:
            return format("{} | Captured piece_t: {} | Promoted To piece_t: {}", out,
                          full_piece_names.at(static_cast<int>(pc2)),
                          full_piece_names.at(static_cast<int>(pc3)));
            break;
    }
}
// NOLINTEND(bugprone-easily-swappable-parameters)
