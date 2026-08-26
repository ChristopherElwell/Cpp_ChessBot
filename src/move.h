#pragma once
#include <cstdint>
#include <string>

#include "bitboard.h"

enum class mov_type : uint8_t
{
    quiet,
    capture,
    promote,
    capture_promote,
    castle_kingside,
    castle_queenside,
    moves_termination
};

inline auto move_type_to_string(mov_type move_type) -> std::string
{
    switch (move_type)
    {
        case mov_type::quiet:
            return "Quiet";
        case mov_type::capture:
            return "Capture";
        case mov_type::promote:
            return "Promote";
        case mov_type::capture_promote:
            return "Capture & Promote";
        case mov_type::castle_kingside:
            return "Castle Kingside";
        case mov_type::castle_queenside:
            return "Castle Queenside";
        case mov_type::moves_termination:
            return "Bookend";
    }
    return "Unknown";
}

constexpr auto operator-(const mov_type type_a, const mov_type type_b) -> int
{
    return static_cast<int>(type_a) - static_cast<int>(type_b);
}

struct Move
{
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    piece_t pc1;
    uint64_t mov1;
    piece_t pc2;
    uint64_t mov2;
    piece_t pc3;
    uint64_t mov3;

    uint64_t info;
    mov_type type;
    // NOLINTEND(misc-non-private-member-variables-in-classes)
    auto operator=(const Move &) -> Move & = default;

    Move();
    Move(piece_t pc1, uint64_t mov1, piece_t pc2, uint64_t mov2, piece_t pc3, uint64_t mov3,
         uint64_t info, mov_type type);

    ~Move() = default;
    Move(const Move &) = default;
    Move(Move &&) = default;
    auto operator=(Move &&) -> Move & = default;

    static auto quiet(piece_t pc1, uint64_t mov1, uint64_t info, uint64_t board_info) -> Move;
    static auto capture(piece_t pc1, uint64_t mov1, piece_t pc2, uint64_t mov2, uint64_t info,
                        uint64_t board_info) -> Move;
    static auto promote(piece_t pc1, uint64_t mov1, piece_t pc2, uint64_t mov2, uint64_t info,
                        uint64_t board_info) -> Move;
    static auto promote_capture(piece_t pc1, uint64_t mov1, piece_t pc2, uint64_t mov2, piece_t pc3,
                                uint64_t mov3, uint64_t info, uint64_t board_info) -> Move;
    static auto castle_kingside(piece_t pc1, uint64_t mov1, piece_t pc2, uint64_t mov2,
                                uint64_t info, uint64_t board_info) -> Move;
    static auto castle_queenside(piece_t pc1, uint64_t mov1, piece_t pc2, uint64_t mov2,
                                 uint64_t info, uint64_t board_info) -> Move;

    [[nodiscard]] auto to_string() const -> std::string;
};
