#ifndef ZOBRIST_H
#define ZOBRIST_H
#include <array>
#include <cstddef>
#include <cstdint>

#include "bitboard_constants.h"

class BitBoard;
class Move;

class ZobristHash
{
public:
    ZobristHash(const BitBoard& board);
    ZobristHash(uint64_t hash);
    ZobristHash() = default;
    void push(const Move& move);
    [[nodiscard]] auto get() const -> uint64_t;

private:
    uint64_t m_hash = 0;

    void push_piece(piece_t piece, uint64_t mask);
    void push_info(uint64_t mask);
};
#endif
