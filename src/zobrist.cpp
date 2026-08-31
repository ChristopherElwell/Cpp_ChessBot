#include "zobrist.h"

#include <bit>
#include <cstdint>

#include "bitboard.h"
#include "bitscan.h"
#include "data.h"
#include "move.h"

using namespace std;

namespace
{
constexpr size_t num_pieces = 12;
constexpr size_t num_squares = 64;
constexpr size_t num_keys = num_pieces * num_squares;
constexpr size_t num_en_passent_keys = 8;
constexpr size_t num_castling_rights_keys = 4;
constexpr size_t num_turn_keys = 1;
constexpr size_t num_info_keys = num_en_passent_keys + num_castling_rights_keys + num_turn_keys;

constexpr size_t turn_key = 0;
constexpr size_t castling_key = 1;
constexpr size_t en_passant_key = 5;

constexpr uint64_t turn_bit = 0b10;

constexpr uint64_t seed = 0xABCDEFABCDEFABCULL;
constexpr uint64_t seed_info = 0xFEDCBFEDCBFEDCBULL;

constexpr uint64_t prime_a = 0x9E3779B97F4A7C15ULL;
constexpr uint64_t prime_b = 0xBF58476D1CE4E5B9ULL;
constexpr uint64_t prime_c = 0x94D049BB133111EBULL;

constexpr uint64_t shift_a = 30;
constexpr uint64_t shift_b = 27;
constexpr uint64_t shift_c = 31;

// splitmix64: simple, fast, statistically solid PRNG — and constexpr-friendly
constexpr auto prng(uint64_t& state) -> uint64_t
{
    uint64_t num = (state += prime_a);
    num = (num ^ (num >> shift_a)) * prime_b;
    num = (num ^ (num >> shift_b)) * prime_c;
    num = (num ^ (num >> shift_c));
    return num;
}

template <size_t Size>
constexpr auto generate_keys(uint64_t seed) -> std::array<uint64_t, Size>
{
    std::array<uint64_t, Size> keys{};
    uint64_t state = seed;

    for (auto& key : keys)
    {
        key = prng(state);
    }

    return keys;
}

constexpr auto zobrist_keys = generate_keys<num_keys>(seed);
constexpr auto zobrist_info_keys = generate_keys<num_info_keys>(seed_info);
}  // namespace

auto ZobristHash::get() const -> uint64_t { return m_hash; }

void ZobristHash::push(const Move& move)
{
    switch (move.type)
    {
        case move_type_t::quiet:
            push_piece(move.pc1, move.mov1);
            break;
        case move_type_t::capture:
        case move_type_t::promote:
        case move_type_t::castle_kingside:
        case move_type_t::castle_queenside:
            push_piece(move.pc1, move.mov1);
            push_piece(move.pc2, move.mov2);
            break;
        case move_type_t::capture_promote:
            push_piece(move.pc1, move.mov1);
            push_piece(move.pc2, move.mov2);
            push_piece(move.pc3, move.mov3);
            break;
        case move_type_t::moves_termination:
            break;
    }

    push_info(move.info | turn_bit);
}

void ZobristHash::push_piece(piece_t piece, uint64_t mask)
{
    for (const uint64_t pos : bit_scan(mask))
    {
        const size_t index = (static_cast<size_t>(piece) * num_squares) + countr_zero(pos);
        m_hash ^= zobrist_keys.at(index);
    }
}

void ZobristHash::push_info(uint64_t mask)
{
    m_hash ^= ((mask & turn_bit) != 0) ? zobrist_info_keys.at(turn_key) : 0;
    m_hash ^=
        ((mask & castling::white_kingside_right) != 0) ? zobrist_info_keys.at(castling_key + 0) : 0;
    m_hash ^= ((mask & castling::white_queenside_right) != 0)
                  ? zobrist_info_keys.at(castling_key + 1)
                  : 0;
    m_hash ^=
        ((mask & castling::black_kingside_right) != 0) ? zobrist_info_keys.at(castling_key + 2) : 0;
    m_hash ^= ((mask & castling::black_queenside_right) != 0)
                  ? zobrist_info_keys.at(castling_key + 3)
                  : 0;
    for (const uint64_t en_passent : bit_scan(mask & ~(masks::rank_1 | masks::rank_8)))
    {
        int en_passent_file = countr_zero(en_passent) % 8;
        m_hash ^= zobrist_info_keys.at(en_passant_key + en_passent_file);
    }
}

ZobristHash::ZobristHash(const BitBoard& board)
{
    for (const piece_t piece : piece_range::all())
    {
        push_piece(piece, board[piece]);
    }
    push_info(board[piece_t::info]);
}

ZobristHash::ZobristHash(uint64_t hash) : m_hash(hash) {}
