#pragma once
#include <cstdint>
#include <string>

#include "bitboard.h"
#include "zobrist.h"

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

inline auto move_type_to_string(move_type_t move_type) -> std::string
{
    switch (move_type)
    {
        case move_type_t::quiet:
            return "Quiet";
        case move_type_t::capture:
            return "Capture";
        case move_type_t::promote_queen:
        case move_type_t::promote_rook:
        case move_type_t::promote_bishop:
        case move_type_t::promote_knight:
            return "Promote";
        case move_type_t::capture_promote_queen:
        case move_type_t::capture_promote_rook:
        case move_type_t::capture_promote_bishop:
        case move_type_t::capture_promote_knight:
            return "Capture & Promote";
        case move_type_t::castle_kingside:
            return "Castle Kingside";
        case move_type_t::castle_queenside:
            return "Castle Queenside";
        case move_type_t::moves_termination:
            return "End of moves";
        case move_type_t::pawn_double:
            return "Pawn double move";
        case move_type_t::en_passent:
            return "En Passent";
    }
    return "Unknown";
}

constexpr auto operator-(const move_type_t type_a, const move_type_t type_b) -> int
{
    return static_cast<int>(type_a) - static_cast<int>(type_b);
}

class Move
{
private:
    uint16_t m_mask;
    static constexpr int to_shift = 4;
    static constexpr int from_shift = 10;
    static constexpr uint16_t type_mask = 0xF;
    static constexpr uint16_t to_mask = 0x3F;
    static constexpr uint16_t from_mask = 0x3F;

public:
    ~Move() = default;
    Move(const Move &) = default;
    Move(Move &&) = default;
    auto operator=(Move &&) -> Move & = default;
    auto operator=(const Move &) -> Move & = default;

    Move(int sq_from = 0, int sq_to = 0, move_type_t type = move_type_t::moves_termination);
    Move(uint64_t sq_from, uint64_t sq_to, move_type_t type = move_type_t::moves_termination);
    [[nodiscard]] auto to() const -> int;
    [[nodiscard]] auto from() const -> int;
    [[nodiscard]] auto type() const -> move_type_t;

    [[nodiscard]] auto is_irreversible() const -> bool;

    [[nodiscard]] auto to_string() const -> std::string;
};
