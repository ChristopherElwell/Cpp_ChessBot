#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace data {

    // TABLE DECLARATIONS
    extern const std::array<int16_t,768> MG_PCTBL;
    extern const std::array<int16_t,768> EG_PCTBL;
    extern const std::array<int,12> MG_EG_PCVALS;
    extern const std::array<const char*,64> SQUARES;
    extern const std::array<const char*, 15> PIECE_NAMES;
    extern const std::array<const char,12> PIECE_CODES;
    extern const std::array<const char*, 7> MOVE_TYPES;
    extern const std::array<uint64_t,64> KING_MOVES;
    extern const std::array<uint64_t,64> KNIGHT_MOVES;
    extern const std::array<uint8_t, 768> SLIDING_MOVES;

}

namespace masks {
    extern const std::array<uint64_t,15> DIAGS_UP;
    extern const std::array<uint64_t,15> DIAGS_DOWN;
    // RANK AND FILES
    inline constexpr uint64_t RANK_1 = 0x00000000000000FF;
    inline constexpr uint64_t RANK_2 = 0x000000000000FF00;
    inline constexpr uint64_t RANK_3 = 0x0000000000FF0000;
    inline constexpr uint64_t RANK_4 = 0x00000000FF000000;
    inline constexpr uint64_t RANK_5 = 0x000000FF00000000;
    inline constexpr uint64_t RANK_6 = 0x0000FF0000000000;
    inline constexpr uint64_t RANK_7 = 0x00FF000000000000;
    inline constexpr uint64_t RANK_8 = 0xFF00000000000000;

    inline constexpr uint64_t FILE_A = 0x8080808080808080;
    inline constexpr uint64_t FILE_B = 0x4040404040404040;
    inline constexpr uint64_t FILE_C = 0x2020202020202020;
    inline constexpr uint64_t FILE_D = 0x1010101010101010;
    inline constexpr uint64_t FILE_E = 0x0808080808080808;
    inline constexpr uint64_t FILE_F = 0x0404040404040404;
    inline constexpr uint64_t FILE_G = 0x0202020202020202;
    inline constexpr uint64_t FILE_H = 0x0101010101010101;

    inline constexpr uint64_t ANTI_DIAG = 0x8040201008040201;

    extern const std::array<uint64_t,8> RANKS;
    extern const std::array<uint64_t,8> FILES;
    extern const std::array<uint64_t,15> DIAGS_DOWN;
    extern const std::array<uint64_t,15> DIAGS_UP;

}
// STRUCTS AND ENUMS

enum class SEARCH_TYPE {
    LOWER_BOUND,
    UPPER_BOUND,
    EXACT
};

inline constexpr int MOVES_ARRAY_LENGTH = 230;
inline constexpr uint64_t TURN_BIT = 0b10;
inline constexpr uint64_t A8 = 0x8000000000000000;

inline const char* pc_chars[12] = { u8"♙", u8"♘", u8"♗", u8"♖", u8"♕", u8"♔",
                             u8"♟", u8"♞", u8"♝", u8"♜", u8"♛", u8"♚" };

inline const char* BLACK_SQ_CHAR = u8"⬛";
inline const char* WHITE_SQ_CHAR = u8"⬜";

namespace castling {
    // CASTLING MASKS
    inline constexpr uint64_t WHITE_KINGSIDE_SPACE = 0b110ULL;
    inline constexpr uint64_t WHITE_QUEENSIDE_SPACE = 0b01110000ULL;
    inline constexpr uint64_t BLACK_KINGSIDE_SPACE = 0b110ULL << 56;
    inline constexpr uint64_t BLACK_QUEENSIDE_SPACE = 0b01110000ULL << 56;

    inline constexpr uint64_t WHITE_KINGSIDE_ATTACKED = 0b1110ULL;
    inline constexpr uint64_t WHITE_QUEENSIDE_ATTACKED = 0b00111000ULL;
    inline constexpr uint64_t BLACK_KINGSIDE_ATTACKED = 0b1110ULL << 56;
    inline constexpr uint64_t BLACK_QUEENSIDE_ATTACKED = 0b00111000ULL << 56;

    inline constexpr uint64_t WHITE_KINGSIDE_RIGHT = 0b00001ULL;
    inline constexpr uint64_t WHITE_QUEENSIDE_RIGHT = 0b10000000ULL;
    inline constexpr uint64_t BLACK_KINGSIDE_RIGHT = 0b1ULL << 56;
    inline constexpr uint64_t BLACK_QUEENSIDE_RIGHT = 0b10000000ULL << 56;
}


