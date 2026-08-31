#ifndef BITBOARD_CONSTANTS_H
#define BITBOARD_CONSTANTS_H

#include <cstdint>
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
    piece_count
};

#endif
