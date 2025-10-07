#pragma once
#include "bitboard.h"
#include <cstdint>
#include <string>

enum class movType : uint8_t
{
    QUIET,
    CAPTURE,
    PROMOTE,
    CAPTURE_PROMOTE,
    CASTLE_KINGSIDE,
    CASTLE_QUEENSIDE,
    BOOK_END
};

inline auto move_type_to_string(movType move_type) -> std::string
{
    switch (move_type)
    {
    case movType::QUIET:
        return "Quiet";
    case movType::CAPTURE:
        return "Capture";
    case movType::PROMOTE:
        return "Promote";
    case movType::CAPTURE_PROMOTE:
        return "Capture & Promote";
    case movType::CASTLE_KINGSIDE:
        return "Castle Kingside";
    case movType::CASTLE_QUEENSIDE:
        return "Castle Queenside";
    case movType::BOOK_END:
        return "Bookend";
    }
    return "Unknown";
}

constexpr auto operator-(const movType type_a, const movType type_b) -> int
{
    return static_cast<int>(type_a) - static_cast<int>(type_b);
}

struct Move
{
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    Piece pc1;
    uint64_t mov1;
    Piece pc2;
    uint64_t mov2;
    Piece pc3;
    uint64_t mov3;

    uint64_t info;
    movType type;
    // NOLINTEND(misc-non-private-member-variables-in-classes)
    auto operator=(const Move &) -> Move & = default;

    Move();
    Move(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2, Piece pc3, uint64_t mov3,
         uint64_t info, movType type);

    ~Move() = default;
    Move(const Move &) = default;
    Move(Move &&) = default;
    auto operator=(Move &&) -> Move & = default;

    static auto quiet(Piece pc1, uint64_t mov1, uint64_t info, uint64_t board_info) -> Move;
    static auto capture(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2, uint64_t info,
                        uint64_t board_info) -> Move;
    static auto promote(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2, uint64_t info,
                        uint64_t board_info) -> Move;
    static auto promote_capture(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2, Piece pc3,
                                uint64_t mov3, uint64_t info, uint64_t board_info) -> Move;
    static auto castle_kingside(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2, uint64_t info,
                                uint64_t board_info) -> Move;
    static auto castle_queenside(Piece pc1, uint64_t mov1, Piece pc2, uint64_t mov2, uint64_t info,
                                 uint64_t board_info) -> Move;
    
    static auto copy(const Move &mov) -> Move;
    [[nodiscard]] auto to_string() const -> std::string;
};
