#ifndef BITBOARD_CONSTANTS_H
#define BITBOARD_CONSTANTS_H

#include <cstdint>

enum class move_type_t : uint8_t
{
    quiet,
    capture,
    promote_queen,
    promote_rook,
    promote_bishop,
    promote_knight,
    capture_promote_queen,
    capture_promote_rook,
    capture_promote_bishop,
    capture_promote_knight,
    castle_kingside,
    castle_queenside,
    pawn_double,
    en_passent,
    moves_termination
};

enum class side_t : uint8_t
{
    white,
    black
};

enum class piece_t : uint8_t
{
    white_pawn,
    white_knight,
    white_bishop,
    white_rook,
    white_queen,
    white_king,
    black_pawn,
    black_knight,
    black_bishop,
    black_rook,
    black_queen,
    black_king,
    white_pcs,
    black_pcs,
    all_pcs,
    info,
    piece_count,
    none,
};

namespace piece
{
template <side_t Side, piece_t WhitePiece>
inline constexpr piece_t piece_for =
    (Side == side_t::white) ? WhitePiece
                            : static_cast<piece_t>(static_cast<uint8_t>(WhitePiece) + 6);

template <side_t Side>
inline constexpr piece_t pawn = piece_for<Side, piece_t::white_pawn>;
template <side_t Side>
inline constexpr piece_t knight = piece_for<Side, piece_t::white_knight>;
template <side_t Side>
inline constexpr piece_t bishop = piece_for<Side, piece_t::white_bishop>;
template <side_t Side>
inline constexpr piece_t rook = piece_for<Side, piece_t::white_rook>;
template <side_t Side>
inline constexpr piece_t queen = piece_for<Side, piece_t::white_queen>;
template <side_t Side>
inline constexpr piece_t king = piece_for<Side, piece_t::white_king>;
template <side_t Side>
inline constexpr piece_t all = piece_t::white_pcs;
template <>
inline constexpr piece_t all<side_t::white> = piece_t::white_pcs;
template <>
inline constexpr piece_t all<side_t::black> = piece_t::black_pcs;
}  // namespace piece
#endif
