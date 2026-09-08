#ifndef BITBOARD_H
#define BITBOARD_H
#include <array>
#include <cstdint>
#include <string>

#include "bitboard_constants.h"
#include "move.h"
#include "zobrist.h"

class Move;

constexpr auto operator~(side_t side) noexcept -> side_t
{
    return side == side_t::white ? side_t::black : side_t::white;
}

inline auto operator-(const piece_t pc_a, const piece_t pc_b) -> int
{
    return static_cast<int>(pc_a) - static_cast<int>(pc_b);
}

struct inv_move
{
    piece_t moving_pc = piece_t::none;
    piece_t captured_pc = piece_t::none;
    uint64_t info = 0ULL;
};

struct piece_range
{
public:
    struct iterator
    {
    private:
        int m_value;

    public:
        auto operator*() const -> piece_t { return static_cast<piece_t>(m_value); }
        auto operator++() -> iterator&
        {
            ++m_value;
            return *this;
        }
        auto operator!=(const iterator& other) const -> bool { return m_value != other.m_value; }
        iterator(int value) : m_value(value) {}
    };

    [[nodiscard]] auto begin() const -> iterator { return iterator{static_cast<int>(m_start)}; }
    [[nodiscard]] auto end() const -> iterator
    {
        const auto end = static_cast<int>(m_stop) + 1;
        return iterator{end};
    }

    piece_range(piece_t start, piece_t stop) : m_start(start), m_stop(stop) {}

    // no color needed — literally every piece, both sides
    static auto all() -> piece_range
    {
        return piece_range{piece_t::white_pawn, piece_t::black_king};
    }

    // color-specific versions, each their own template
    template <side_t Side>
    static auto all() -> piece_range
    {
        if constexpr (Side == side_t::white)
        {
            return piece_range{piece_t::white_pawn, piece_t::white_king};
        }
        else
        {
            return piece_range{piece_t::black_pawn, piece_t::black_king};
        }
    }

    template <side_t Side>
    static auto no_king() -> piece_range
    {
        if constexpr (Side == side_t::white)
        {
            return piece_range{piece_t::white_pawn, piece_t::white_queen};
        }
        else
        {
            return piece_range{piece_t::black_pawn, piece_t::black_queen};
        }
    }

private:
    piece_t m_start;
    piece_t m_stop;
};
;
;

class BitBoard
{
private:
    std::array<uint64_t, static_cast<int>(piece_t::piece_count)> m_board{};
    ZobristHash m_hash;

    static constexpr uint64_t turn_bit = 0b10000;
    static auto sq_from_name(char file, char rank) -> uint64_t;
    template <side_t Side>
    void apply_mask(piece_t piece, uint64_t mask);

    template <side_t Side>
    auto mask_move(move_type_t type, piece_t moving_pc, uint64_t from_mask, uint64_t to_mask,
                   piece_t captured_pc = piece_t::none);

public:
    static auto start_position() -> BitBoard;
    BitBoard(const std::string& fen);
    [[nodiscard]] auto to_fen() const -> std::string;
    BitBoard();
    [[nodiscard]] auto draw() const -> std::string;
    auto operator[](piece_t piece) const -> uint64_t;

    template <side_t Side>
    auto apply_move(const Move& move) -> inv_move;
    auto apply_move(const Move& move) -> inv_move;

    template <side_t Side>
    void undo_move(const Move& move, const inv_move& inverse);
    void undo_move(const Move& move, const inv_move& inverse);

    [[nodiscard]] auto side_to_move() const -> side_t
    {
        return (m_board[static_cast<int>(piece_t::info)] & turn_bit) != 0 ? side_t::white
                                                                          : side_t::black;
    }
    [[nodiscard]] auto hash() const -> ZobristHash;
    [[nodiscard]] auto piece_at(int pos) const -> piece_t;
    [[nodiscard]] auto piece_at(uint64_t mask) const -> piece_t;
};

#endif
