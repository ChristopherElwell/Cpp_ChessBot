#ifndef ZOBRIST_H
#define ZOBRIST_H
#include <array>
#include <cstddef>
#include <cstdint>

#include "bitboard_constants.h"

class BitBoard;
struct move;

class ZobristHash
{
public:
    ZobristHash(const BitBoard& board);
    ZobristHash(uint64_t hash);
    ZobristHash() = default;
    [[nodiscard]] auto get() const -> uint64_t;

    void push_piece(piece_t piece, uint64_t mask);
    void push_info(uint64_t mask);

private:
    uint64_t m_hash = 0;
};
#endif
