#pragma once
#include <cstdint>
#include <string>

#include "bitboard.h"

enum class movType : uint8_t
{
    QUIET,
    CAPTURE,
    PROMOTE,
    CAPTURE_PROMOTE,
    CASTLE_kingside,
    CASTLE_queenside,
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
        case movType::CASTLE_kingside:
            return "Castle Kingside";
        case movType::CASTLE_queenside:
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
    piece_t pc1;
    uint64_t mov1;
    piece_t pc2;
    uint64_t mov2;
    piece_t pc3;
    uint64_t mov3;

    uint64_t info;
    movType type;
    // NOLINTEND(misc-non-private-member-variables-in-classes)
    auto operator=(const Move &) -> Move & = default;

    Move();
    Move(piece_t pc1, uint64_t mov1, piece_t pc2, uint64_t mov2, piece_t pc3, uint64_t mov3,
         uint64_t info, movType type);

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

    static auto copy(const Move &mov) -> Move;
    [[nodiscard]] auto to_string() const -> std::string;
};
