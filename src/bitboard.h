#pragma once
#include <array>
#include <cstdint>
#include <string>

struct Move;

enum class side_t : uint8_t
{
    white,
    black
};

constexpr auto operator~(side_t side) noexcept -> side_t
{
    return side == side_t::white ? side_t::black : side_t::white;
}

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

inline auto operator-(const piece_t pc_a, const piece_t pc_b) -> int
{
    return static_cast<int>(pc_a) - static_cast<int>(pc_b);
}

struct piece_range
{
   public:
    struct iterator
    {
       private:
        int m_value;

       public:
        auto operator*() const -> piece_t { return static_cast<piece_t>(m_value); }
        auto operator++() -> iterator &
        {
            ++m_value;
            return *this;
        }
        auto operator!=(const iterator &other) const -> bool { return m_value != other.m_value; }
        iterator(int value) : m_value(value) {}
    };

    [[nodiscard]] auto begin() const -> iterator { return iterator{static_cast<int>(m_start)}; }
    [[nodiscard]] auto end() const -> iterator
    {
        const auto end = static_cast<int>(m_stop) + 1;
        return iterator{end};
    }

    piece_range(piece_t start, piece_t stop) : m_start(start), m_stop(stop) {};
    static auto black() -> piece_range
    {
        return piece_range{piece_t::black_pawn, piece_t::black_king};
    }
    static auto white() -> piece_range
    {
        return piece_range{piece_t::white_pawn, piece_t::white_king};
    }
    static auto black_no_king() -> piece_range
    {
        return piece_range{piece_t::black_pawn, piece_t::black_queen};
    }
    static auto white_no_king() -> piece_range
    {
        return piece_range{piece_t::white_pawn, piece_t::white_queen};
    }
    static auto all() -> piece_range
    {
        return piece_range{piece_t::white_pawn, piece_t::black_king};
    }

   private:
    piece_t m_start;
    piece_t m_stop;
};

class BitBoard
{
   private:
    std::array<uint64_t, static_cast<int>(piece_t::piece_count)> m_board{};
    static constexpr uint64_t turn_bit = 0b10;

    static auto sq_from_name(char file, char rank) -> uint64_t;

   public:
    static constexpr int num_squares = 64;

    static auto start_position() -> BitBoard;
    BitBoard(const std::string &fen);
    BitBoard();
    [[nodiscard]] auto draw() const -> std::string;
    auto operator[](piece_t piece) const -> uint64_t;
    void apply_move(const Move &move);
    [[nodiscard]] auto whites_turn() const -> bool
    {
        return (m_board[static_cast<int>(piece_t::info)] & turn_bit) != 0;
    }
};
