#pragma once
#include "data.h"
#include <array>
#include <cstdint>
#include <string>

struct Move;

enum class side_t : uint8_t {
  white,
  black
};

constexpr auto operator~(side_t side) noexcept -> side_t {
    return side == side_t::white ? side_t::black : side_t::white;
}

enum class Piece : uint8_t
{
    WHITE_PAWN,
    WHITE_KNIGHT,
    WHITE_BISHOP,
    WHITE_ROOK,
    WHITE_QUEEN,
    WHITE_KING,
    BLACK_PAWN,
    BLACK_KNIGHT,
    BLACK_BISHOP,
    BLACK_ROOK,
    BLACK_QUEEN,
    BLACK_KING,
    WHITE_PCS,
    BLACK_PCS,
    ALL_PCS,
    INFO,
    PIECECOUNT
};

inline auto pc_to_string(const Piece piece) -> std::string
{
    return data::PIECE_NAMES[static_cast<int>(piece)];
}

inline auto operator-(const Piece pc_a, const Piece pc_b) -> int
{
    return static_cast<int>(pc_a) - static_cast<int>(pc_b);
}

struct PieceRange
{
  public:
    struct Iterator
    {
      private:
        int value;

      public:
        auto operator*() const -> Piece { return static_cast<Piece>(value); }
        auto operator++() -> Iterator &
        {
            ++value;
            return *this;
        }
        auto operator!=(const Iterator &other) const -> bool { return value != other.value; }
        Iterator(int value) : value(value) {}
    };

    [[nodiscard]] auto begin() const -> Iterator { return Iterator{static_cast<int>(start)}; }
    [[nodiscard]] auto end() const -> Iterator
    {
        const auto end = static_cast<int>(stop) + 1;
        return Iterator{end};
    }

    PieceRange(Piece start, Piece stop) : start(start), stop(stop) {};
    static auto Black() -> PieceRange { return PieceRange{Piece::BLACK_PAWN, Piece::BLACK_KING}; }
    static auto White() -> PieceRange { return PieceRange{Piece::WHITE_PAWN, Piece::WHITE_KING}; }
    static auto BlackNoKing() -> PieceRange
    {
        return PieceRange{Piece::BLACK_PAWN, Piece::BLACK_QUEEN};
    }
    static auto WhiteNoKing() -> PieceRange
    {
        return PieceRange{Piece::WHITE_PAWN, Piece::WHITE_QUEEN};
    }
    static auto all() -> PieceRange { return PieceRange{Piece::WHITE_PAWN, Piece::BLACK_KING}; }

  private:
    Piece start;
    Piece stop;
};

class BitBoard
{
  private:
    std::array<uint64_t, static_cast<int>(Piece::PIECECOUNT)> board{};
    static constexpr uint64_t TURN_BIT = 0b10;

    static auto sq_from_name(char file, char rank) -> uint64_t;

  public:
    static auto start_position() -> BitBoard;
    BitBoard(const std::string &FEN);
    BitBoard();
    [[nodiscard]] auto draw() const -> std::string;
    auto operator[](Piece piece) const -> uint64_t;
    void apply_move(const Move &move);
    [[nodiscard]] auto whites_turn() const -> bool
    {
        return (board[static_cast<int>(Piece::INFO)] & TURN_BIT) != 0;
    }
};