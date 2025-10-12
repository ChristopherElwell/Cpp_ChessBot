#include "move_gen.h"
#include "bitboard.h"
#include "bitscan.h"
#include "data.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

using namespace std;

auto MoveGen::get_white_rook_attacks(const uint64_t rook) const -> uint64_t
{
    const int pos = __builtin_ctzll(rook);
    const int rank = pos >> 3;
    const int file = pos & 7;
    const int base = pos & ~7;
    uint64_t attacks =
        (uint64_t)(move_masks::sliding_moves[(((m_board[piece_t::all_pcs] >> (base + 1)) & 0x3F) << 3) +
                                       file])
        << base;

    const uint64_t file_isolated = m_board[piece_t::all_pcs] << (8 - file) & masks::file_H;
    const uint64_t rotated = (file_isolated * masks::anti_diag) >> 56;
    const uint64_t index = (rotated * 8 + (7 - rank)) & 0x1ff;
    const uint64_t moves_rotated = ((uint64_t)move_masks::sliding_moves[index]) * masks::anti_diag;
    attacks |= (moves_rotated & masks::file_A) >> (7 - file);

    return attacks & ~m_board[piece_t::white_pcs];
}

auto MoveGen::get_black_rook_attacks(const uint64_t rook) const -> uint64_t
{
    const int pos = __builtin_ctzll(rook);
    const int rank = pos >> 3;
    const int file = pos & 7;
    const int base = pos & ~7;

    uint64_t attacks =
        (uint64_t)(move_masks::sliding_moves[(((m_board[piece_t::all_pcs] >> (base + 1)) & 0x3F) << 3) +
                                       file])
        << base;

    const uint64_t file_isolated = m_board[piece_t::all_pcs] << (8 - file) & masks::file_H;
    const uint64_t rotated = (file_isolated * masks::anti_diag) >> 56;
    const uint64_t index = (rotated * 8 + (7 - rank)) & 0x1ff;
    const uint64_t moves_rotated = ((uint64_t)move_masks::sliding_moves[index]) * masks::anti_diag;
    attacks |= (moves_rotated & masks::file_A) >> (7 - file);

    return attacks & ~m_board[piece_t::black_pcs];
}

auto MoveGen::get_white_bishop_attacks(const uint64_t bishop) const -> uint64_t
{

    uint64_t up_ray = 0;
    uint64_t down_ray = 0;
    uint64_t mask = 0;
    uint64_t temp = 0;
    uint64_t first_pc = 0;
    uint64_t spots = 0;
    const int pos = __builtin_ctzll(bishop);
    up_ray = masks::diag_up[(pos & 7) + (pos >> 3)];
    down_ray = masks::diag_down[7 + (pos >> 3) - (pos & 7)];

    const uint64_t sqs_ahead = ~((bishop - 1) | bishop);
    const uint64_t pcs_ahead = m_board[piece_t::all_pcs] & sqs_ahead;
    const uint64_t pcs_behind = m_board[piece_t::all_pcs] & (bishop - 1);

    first_pc = pcs_ahead & -(pcs_ahead & up_ray) & up_ray;
    spots = ((first_pc - 1) & sqs_ahead & up_ray) | (first_pc & m_board[piece_t::black_pcs]);

    temp = pcs_behind & up_ray;
    mask = static_cast<int>(temp == 0) - 1;
    first_pc = (sq_A8 >> __builtin_clzll(temp)) & mask;
    spots |= (bishop - 1) & ~((first_pc - 1) | first_pc) & up_ray;
    spots |= first_pc & m_board[piece_t::black_pcs];
    spots |= up_ray & (bishop - 1) & ~mask;

    first_pc = pcs_ahead & -(pcs_ahead & down_ray) & down_ray;
    spots |= ((first_pc - 1) & sqs_ahead & down_ray) | (first_pc & m_board[piece_t::black_pcs]);

    temp = pcs_behind & down_ray;
    mask = static_cast<int>(temp == 0) - 1;
    first_pc = (sq_A8 >> __builtin_clzll(temp)) & mask;
    spots |= (bishop - 1) & ~((first_pc - 1) | first_pc) & down_ray;
    spots |= first_pc & m_board[piece_t::black_pcs];
    spots |= down_ray & (bishop - 1) & ~mask;
    return spots;
}

auto MoveGen::get_black_bishop_attacks(const uint64_t bishop) const -> uint64_t
{
    uint64_t up_ray = 0;
    uint64_t down_ray = 0;
    uint64_t mask = 0;
    uint64_t temp = 0;
    uint64_t first_pc = 0;
    uint64_t spots = 0;
    const int pos = __builtin_ctzll(bishop);
    up_ray = masks::diag_up[(pos & 7) + (pos >> 3)];
    down_ray = masks::diag_down[7 + (pos >> 3) - (pos & 7)];

    const uint64_t sqs_ahead = ~((bishop - 1) | bishop);
    const uint64_t pcs_ahead = m_board[piece_t::all_pcs] & sqs_ahead;
    const uint64_t pcs_behind = m_board[piece_t::all_pcs] & (bishop - 1);

    first_pc = pcs_ahead & -(pcs_ahead & up_ray) & up_ray;
    spots = ((first_pc - 1) & sqs_ahead & up_ray) | (first_pc & m_board[piece_t::white_pcs]);

    temp = pcs_behind & up_ray;
    mask = static_cast<int>(temp == 0) - 1;
    first_pc = (sq_A8 >> __builtin_clzll(temp)) & mask;
    spots |= (bishop - 1) & ~((first_pc - 1) | first_pc) & up_ray;
    spots |= first_pc & m_board[piece_t::white_pcs];
    spots |= up_ray & (bishop - 1) & ~mask;

    first_pc = pcs_ahead & -(pcs_ahead & down_ray) & down_ray;
    spots |= ((first_pc - 1) & sqs_ahead & down_ray) | (first_pc & m_board[piece_t::white_pcs]);

    temp = pcs_behind & down_ray;
    mask = static_cast<int>(temp == 0) - 1;
    first_pc = (sq_A8 >> __builtin_clzll(temp)) & mask;
    spots |= (bishop - 1) & ~((first_pc - 1) | first_pc) & down_ray;
    spots |= first_pc & m_board[piece_t::white_pcs];
    spots |= down_ray & (bishop - 1) & ~mask;
    return spots;
}

void MoveGen::get_white_knight_moves()
{
    for (const auto knight : BitScan(m_board[piece_t::white_knight]))
    {
        const uint64_t moves =
            move_masks::knight_moves[__builtin_ctzll(knight)] & ~m_board[piece_t::white_pcs];
        white_add_to_movs(piece_t::white_knight, knight, moves, 0);
    }
}

void MoveGen::get_black_knight_moves()
{
    for (const auto knight : BitScan(m_board[piece_t::black_knight]))
    {
        const uint64_t moves =
            move_masks::knight_moves[__builtin_ctzll(knight)] & ~m_board[piece_t::black_pcs];
        black_add_to_movs(piece_t::black_knight, knight, moves, 0);
    }
}

void MoveGen::get_white_rook_moves()
{
    for (const auto rook : BitScan(m_board[piece_t::white_rook]))
    {
        const uint64_t moves = get_white_rook_attacks(rook);
        const uint64_t info = m_board[piece_t::info] & rook &
                              (castling::white_kingside_right | castling::white_queenside_right);
        white_add_to_movs(piece_t::white_rook, rook, moves, info);
    }
}

void MoveGen::get_black_rook_moves()
{
    for (const auto rook : BitScan(m_board[piece_t::black_rook]))
    {
        const uint64_t moves = get_black_rook_attacks(rook);
        const uint64_t info = m_board[piece_t::info] & rook &
                              (castling::black_kingside_right | castling::black_queenside_right);
        black_add_to_movs(piece_t::black_rook, rook, moves, info);
    }
}

void MoveGen::get_white_bishop_moves()
{
    for (const auto bishop : BitScan(m_board[piece_t::white_bishop]))
    {
        const uint64_t moves = get_white_bishop_attacks(bishop);

        white_add_to_movs(piece_t::white_bishop, bishop, moves, 0);
    }
}

void MoveGen::get_black_bishop_moves()
{
    for (const auto bishop : BitScan(m_board[piece_t::black_bishop]))
    {
        const uint64_t moves = get_black_bishop_attacks(bishop);

        black_add_to_movs(piece_t::black_bishop, bishop, moves, 0);
    }
}

void MoveGen::get_white_pawn_moves()
{
    for (const auto one_step :
         BitScan((m_board[piece_t::white_pawn] << 8) & ~masks::rank_8 & ~m_board[piece_t::all_pcs]))
    {
        m_movs[m_idx++] =
            Move::quiet(piece_t::white_pawn, one_step | one_step >> 8, 0, m_board[piece_t::info]);
    }
    for (const auto one_step_prom :
         BitScan((m_board[piece_t::white_pawn] << 8) & masks::rank_8 & ~m_board[piece_t::all_pcs]))
    {
        m_movs[m_idx++] = Move::promote(piece_t::white_pawn, one_step_prom >> 8, piece_t::white_queen,
                                        one_step_prom, 0, m_board[piece_t::info]);
        m_movs[m_idx++] = Move::promote(piece_t::white_pawn, one_step_prom >> 8, piece_t::white_knight,
                                        one_step_prom, 0, m_board[piece_t::info]);
        m_movs[m_idx++] = Move::promote(piece_t::white_pawn, one_step_prom >> 8, piece_t::white_rook,
                                        one_step_prom, 0, m_board[piece_t::info]);
        m_movs[m_idx++] = Move::promote(piece_t::white_pawn, one_step_prom >> 8, piece_t::white_bishop,
                                        one_step_prom, 0, m_board[piece_t::info]);
    }

    const uint64_t two_steps = ((m_board[piece_t::white_pawn] & masks::rank_2) << 16) &
                               ~((m_board[piece_t::all_pcs]) | (m_board[piece_t::all_pcs] << 8));
    for (const auto two_step : BitScan(two_steps))
    {
        m_movs[m_idx++] = Move::quiet(piece_t::white_pawn, two_step | two_step >> 16, (two_step >> 8),
                                      m_board[piece_t::info]);
    }

    white_pawn_taking_moves(7);
    white_pawn_taking_moves(9);

    const uint64_t en_passent_take_left = ((m_board[piece_t::white_pawn] << 9) &
                                           m_board[piece_t::info] & ~masks::rank_1 & ~masks::file_H);
    if (en_passent_take_left != 0)
    {
        m_movs[m_idx++] =
            Move::capture(piece_t::white_pawn, en_passent_take_left | (en_passent_take_left >> 9),
                          piece_t::black_pawn, en_passent_take_left >> 8, 0, m_board[piece_t::info]);
    }

    const uint64_t en_passent_take_right = ((m_board[piece_t::white_pawn] << 7) &
                                            m_board[piece_t::info] & ~masks::rank_1 & ~masks::file_A);
    if (en_passent_take_right != 0)
    {
        m_movs[m_idx++] =
            Move::capture(piece_t::white_pawn, en_passent_take_right | (en_passent_take_right >> 7),
                          piece_t::black_pawn, en_passent_take_right >> 8, 0, m_board[piece_t::info]);
    }
}

void MoveGen::get_black_pawn_moves()
{

    for (const auto one_step :
         BitScan((m_board[piece_t::black_pawn] >> 8) & ~masks::rank_1 & ~m_board[piece_t::all_pcs]))
    {
        m_movs[m_idx++] =
            Move::quiet(piece_t::black_pawn, one_step | one_step << 8, 0, m_board[piece_t::info]);
    }

    for (const auto one_step_prom :
         BitScan((m_board[piece_t::black_pawn] >> 8) & masks::rank_1 & ~m_board[piece_t::all_pcs]))
    {
        m_movs[m_idx++] = Move::promote(piece_t::black_pawn, one_step_prom << 8, piece_t::black_queen,
                                        one_step_prom, 0, m_board[piece_t::info]);
        m_movs[m_idx++] = Move::promote(piece_t::black_pawn, one_step_prom << 8, piece_t::black_knight,
                                        one_step_prom, 0, m_board[piece_t::info]);
        m_movs[m_idx++] = Move::promote(piece_t::black_pawn, one_step_prom << 8, piece_t::black_rook,
                                        one_step_prom, 0, m_board[piece_t::info]);
        m_movs[m_idx++] = Move::promote(piece_t::black_pawn, one_step_prom << 8, piece_t::black_bishop,
                                        one_step_prom, 0, m_board[piece_t::info]);
    }

    const uint64_t two_steps = ((m_board[piece_t::black_pawn] & masks::rank_7) >> 16) &
                               ~((m_board[piece_t::all_pcs]) | (m_board[piece_t::all_pcs] >> 8));
    for (const auto two_step : BitScan(two_steps))
    {
        m_movs[m_idx++] = Move::quiet(piece_t::black_pawn, two_step | two_step << 16, (two_step << 8),
                                      m_board[piece_t::info]);
    }

    black_pawn_taking_moves(9);
    black_pawn_taking_moves(7);
    const uint64_t en_passent_take_left = ((m_board[piece_t::black_pawn] >> 7) &
                                           m_board[piece_t::info] & ~masks::rank_1 & ~masks::file_H);
    if (en_passent_take_left != 0)
    {
        m_movs[m_idx++] =
            Move::capture(piece_t::black_pawn, en_passent_take_left | (en_passent_take_left << 7),
                          piece_t::white_pawn, en_passent_take_left << 8, 0, m_board[piece_t::info]);
    }

    const uint64_t en_passent_take_right = ((m_board[piece_t::black_pawn] >> 9) &
                                            m_board[piece_t::info] & ~masks::rank_1 & ~masks::file_A);
    if (en_passent_take_right != 0)
    {
        m_movs[m_idx++] =
            Move::capture(piece_t::black_pawn, en_passent_take_right | (en_passent_take_right << 9),
                          piece_t::white_pawn, en_passent_take_right << 8, 0, m_board[piece_t::info]);
    }
}

void MoveGen::black_pawn_taking_moves(const int offset)
{
    uint64_t file_mask = offset == 7 ? masks::file_H : masks::file_A;
    for (const auto take_right :
         BitScan((m_board[piece_t::black_pawn] >> offset) & m_board[piece_t::white_pcs] & ~file_mask))
    {
        for (auto const piece : piece_range::WhiteNoKing())
        {
            const uint64_t taken_piece = (take_right & m_board[piece]);
            if (taken_piece == 0)
            {
                continue;
            }
            const uint64_t promotion_sq = (take_right & masks::rank_1);
            if (promotion_sq != 0)
            {
                m_movs[m_idx++] = Move::promote_capture(piece_t::black_pawn, take_right << offset,
                                                        piece, taken_piece, piece_t::black_queen,
                                                        promotion_sq, 0, m_board[piece_t::info]);
                m_movs[m_idx++] = Move::promote_capture(piece_t::black_pawn, take_right << offset,
                                                        piece, taken_piece, piece_t::black_knight,
                                                        promotion_sq, 0, m_board[piece_t::info]);
                m_movs[m_idx++] = Move::promote_capture(piece_t::black_pawn, take_right << offset,
                                                        piece, taken_piece, piece_t::black_bishop,
                                                        promotion_sq, 0, m_board[piece_t::info]);
                m_movs[m_idx++] = Move::promote_capture(piece_t::black_pawn, take_right << offset,
                                                        piece, taken_piece, piece_t::black_rook,
                                                        promotion_sq, 0, m_board[piece_t::info]);
            }
            else
            {
                m_movs[m_idx++] =
                    Move::capture(piece_t::black_pawn, take_right | take_right << offset, piece,
                                  taken_piece, 0, m_board[piece_t::info]);
            }
            break;
        }
    }
}

void MoveGen::white_pawn_taking_moves(const int offset)
{
    uint64_t file_mask = offset == 7 ? masks::file_A : masks::file_H;
    for (const auto take :
         BitScan((m_board[piece_t::white_pawn] << offset) & m_board[piece_t::black_pcs] & ~file_mask))
    {
        for (auto const piece : piece_range::BlackNoKing())
        {
            const uint64_t taken_piece = (take & m_board[piece]);
            if (taken_piece == 0)
            {
                continue;
            }
            const uint64_t promotion_sq = (take & masks::rank_8);
            if (promotion_sq != 0)
            {
                m_movs[m_idx++] = Move::promote_capture(piece_t::white_pawn, take >> offset, piece,
                                                        taken_piece, piece_t::white_queen,
                                                        promotion_sq, 0, m_board[piece_t::info]);
                m_movs[m_idx++] = Move::promote_capture(piece_t::white_pawn, take >> offset, piece,
                                                        taken_piece, piece_t::white_knight,
                                                        promotion_sq, 0, m_board[piece_t::info]);
                m_movs[m_idx++] = Move::promote_capture(piece_t::white_pawn, take >> offset, piece,
                                                        taken_piece, piece_t::white_bishop,
                                                        promotion_sq, 0, m_board[piece_t::info]);
                m_movs[m_idx++] =
                    Move::promote_capture(piece_t::white_pawn, take >> offset, piece, taken_piece,
                                          piece_t::white_rook, promotion_sq, 0, m_board[piece_t::info]);
            }
            else
            {
                m_movs[m_idx++] = Move::capture(piece_t::white_pawn, take | take >> offset, piece,
                                                taken_piece, 0, m_board[piece_t::info]);
            }
            break;
        }
    }
}

void MoveGen::get_white_king_moves()
{
    const uint64_t moves =
        move_masks::king_moves[__builtin_ctzll(m_board[piece_t::white_king])] & ~m_board[piece_t::white_pcs];
    const uint64_t info_xor =
        (castling::white_kingside_right | castling::white_queenside_right) & m_board[piece_t::info];

    white_add_to_movs(piece_t::white_king, m_board[piece_t::white_king], moves, info_xor);

    uint64_t attacks = 0;
    if (((m_board[piece_t::info] & castling::white_kingside_right) != 0) &&
        ((castling::white_kingside_space &
          (m_board[piece_t::white_pcs] | m_board[piece_t::black_pcs])) == 0) &&
        ((m_board[piece_t::white_rook] & masks::file_H & masks::rank_1) != 0))
    {
        attacks = get_black_attackers(m_board);
        if ((attacks & castling::white_kingside_attacked) == 0)
        {
            m_movs[m_idx++] = Move::castle_kingside(piece_t::white_king, 0b1010ULL, piece_t::white_rook,
                                                    0b0101ULL, info_xor, m_board[piece_t::info]);
        }
    }
    if (((m_board[piece_t::info] & castling::white_queenside_right) != 0) &&
        ((castling::white_queenside_space &
          (m_board[piece_t::white_pcs] | m_board[piece_t::black_pcs])) == 0) &&
        ((m_board[piece_t::white_rook] & masks::file_A & masks::rank_1) != 0))
    {
        if (attacks == 0)
        {
            attacks = get_black_attackers(m_board);
        }

        if ((attacks & castling::white_queenside_attacked) == 0)
        {
            m_movs[m_idx++] =
                Move::castle_queenside(piece_t::white_king, 0b101000ULL, piece_t::white_rook,
                                       0b10010000ULL, info_xor, m_board[piece_t::info]);
        }
    }
}

void MoveGen::get_black_king_moves()
{
    const uint64_t moves =
        move_masks::king_moves[__builtin_ctzll(m_board[piece_t::black_king])] & ~m_board[piece_t::black_pcs];

    const uint64_t info_xor =
        (castling::black_kingside_right | castling::black_queenside_right) & m_board[piece_t::info];

    black_add_to_movs(piece_t::black_king, m_board[piece_t::black_king], moves, info_xor);

    uint64_t attacks = 0;
    if (((m_board[piece_t::info] & castling::black_kingside_right) != 0) &&
        ((castling::black_kingside_space &
          (m_board[piece_t::white_pcs] | m_board[piece_t::black_pcs])) == 0) &&
        ((m_board[piece_t::black_rook] & masks::file_H & masks::rank_8) != 0))
    {
        attacks = get_white_attackers(m_board);
        if ((attacks & castling::black_kingside_attacked) == 0)
        {
            m_movs[m_idx++] =
                Move::castle_kingside(piece_t::black_king, 0b1010ULL << 56, piece_t::black_rook,
                                      0b0101ULL << 56, info_xor, m_board[piece_t::info]);
        }
    }

    if (((m_board[piece_t::info] & castling::black_queenside_right) != 0) &&
        ((castling::black_queenside_space &
          (m_board[piece_t::white_pcs] | m_board[piece_t::black_pcs])) == 0) &&
        ((m_board[piece_t::black_rook] & masks::file_A & masks::rank_8) != 0))
    {
        if (attacks == 0)
        {
            attacks = get_white_attackers(m_board);
        }

        if ((attacks & castling::black_queenside_attacked) == 0)
        {
            m_movs[m_idx++] =
                Move::castle_queenside(piece_t::black_king, 0b101000ULL << 56, piece_t::black_rook,
                                       0b10010000ULL << 56, info_xor, m_board[piece_t::info]);
        }
    }
}

void MoveGen::get_white_queen_moves()
{
    for (const auto queen : BitScan(m_board[piece_t::white_queen]))
    {
        const uint64_t moves = get_white_bishop_attacks(queen) | get_white_rook_attacks(queen);

        white_add_to_movs(piece_t::white_queen, queen, moves, 0);
    }
}

void MoveGen::get_black_queen_moves()
{
    for (const auto queen : BitScan(m_board[piece_t::black_queen]))
    {
        const uint64_t moves = get_black_bishop_attacks(queen) | get_black_rook_attacks(queen);

        black_add_to_movs(piece_t::black_queen, queen, moves, 0);
    }
}

void MoveGen::white_add_to_movs(const piece_t moving_pc, const uint64_t moving_pc_spot,
                                const uint64_t moves, const uint64_t info)
{
    const uint64_t board_info = m_board[piece_t::info];

    for (const auto mov : BitScan(moves & ~m_board[piece_t::black_pcs]))
    {
        m_movs[m_idx++] = Move::quiet(moving_pc, mov | moving_pc_spot, info, board_info);
    }

    for (const auto taking_spot : BitScan(moves & m_board[piece_t::black_pcs]))
    {
        for (const auto taken_pc : piece_range::BlackNoKing())
        {
            const uint64_t taken_spot = (taking_spot & m_board[taken_pc]);
            if (taken_spot != 0)
            {
                m_movs[m_idx++] = Move::capture(moving_pc, taken_spot | moving_pc_spot, taken_pc,
                                                taken_spot, info, board_info);
                break;
            }
        }
    }
}

void MoveGen::black_add_to_movs(const piece_t moving_pc, const uint64_t moving_pc_spot,
                                const uint64_t moves, const uint64_t info)
{
    const uint64_t board_info = m_board[piece_t::info];
    for (const auto mov : BitScan(moves & ~m_board[piece_t::white_pcs]))
    {
        m_movs[m_idx++] = Move::quiet(moving_pc, mov | moving_pc_spot, info, board_info);
    }

    for (const auto taking_spot : BitScan(moves & m_board[piece_t::white_pcs]))
    {
        for (const auto taken_pc : piece_range::WhiteNoKing())
        {
            const uint64_t taken_spot = (taking_spot & m_board[taken_pc]);
            if (taken_spot != 0)
            {
                m_movs[m_idx++] = Move::capture(moving_pc, taken_spot | moving_pc_spot, taken_pc,
                                                taken_spot, info, board_info);
                break;
            }
        }
    }
}

auto MoveGen::get_white_attackers(const BitBoard &m_board) -> uint64_t
{
    uint64_t attacks = 0;
    attacks |= ((m_board[piece_t::white_pawn] & ~masks::file_H) << 7) |
               ((m_board[piece_t::white_pawn] & ~masks::file_A) << 9);

    attacks |= move_masks::king_moves[__builtin_ctzll(m_board[piece_t::white_king])];

    for (const auto piece : BitScan(m_board[piece_t::white_knight]))
    {
        attacks |= move_masks::knight_moves[__builtin_ctzll(piece)];
    }

    for (const auto piece : BitScan((m_board[piece_t::white_bishop] | m_board[piece_t::white_queen])))
    {
        attacks |= get_white_bishop_attacks(piece);
    }

    for (const auto piece : BitScan((m_board[piece_t::white_rook] | m_board[piece_t::white_queen])))
    {
        attacks |= get_white_rook_attacks(piece);
    }

    return attacks;
}

auto MoveGen::get_black_attackers(const BitBoard &m_board) -> uint64_t
{

    uint64_t attacks = 0;

    attacks |= ((m_board[piece_t::black_pawn] & ~masks::file_H) >> 9) |
               ((m_board[piece_t::black_pawn] & ~masks::file_A) >> 7);

    attacks |= move_masks::king_moves[__builtin_ctzll(m_board[piece_t::black_king])];

    for (const auto piece : BitScan(m_board[piece_t::black_knight]))
    {
        attacks |= move_masks::knight_moves[__builtin_ctzll(piece)];
    }

    for (const auto piece : BitScan(m_board[piece_t::black_bishop] | m_board[piece_t::black_queen]))
    {
        attacks |= get_black_bishop_attacks(piece);
    }

    for (const auto piece : BitScan(m_board[piece_t::black_rook] | m_board[piece_t::black_queen]))
    {
        attacks |= get_black_rook_attacks(piece);
    }

    return attacks;
}

// NOLINTBEGIN
auto MoveGen::is_white_king_in_check() const -> bool
{
    if (((m_board[piece_t::black_rook] | m_board[piece_t::black_queen]) &
         get_white_rook_attacks(m_board[piece_t::white_king])) != 0)
    {
        return true;
    }
    if (((m_board[piece_t::black_bishop] | m_board[piece_t::black_queen]) &
         get_white_bishop_attacks(m_board[piece_t::white_king])) != 0)
    {
        return true;
    }
    if ((m_board[piece_t::black_knight] &
         move_masks::knight_moves[__builtin_ctzll(m_board[piece_t::white_king])]) != 0)
    {
        return true;
    }
    if ((m_board[piece_t::black_king] &
         move_masks::king_moves[__builtin_ctzll(m_board[piece_t::white_king])]) != 0)
    {
        return true;
    }
    if (((((m_board[piece_t::white_king] << 9) & ~masks::file_H) |
          ((m_board[piece_t::white_king] << 7) & ~masks::file_A)) &
         m_board[piece_t::black_pawn]) != 0)
    {
        return true;
    }
    return false;
}

auto MoveGen::is_black_king_in_check() const -> bool
{
    if (((m_board[piece_t::white_rook] | m_board[piece_t::white_queen]) &
         get_black_rook_attacks(m_board[piece_t::black_king])) != 0)
    {
        return true;
    }
    if (((m_board[piece_t::white_bishop] | m_board[piece_t::white_queen]) &
         get_black_bishop_attacks(m_board[piece_t::black_king])) != 0)
    {
        return true;
    }
    if ((m_board[piece_t::white_knight] &
         move_masks::knight_moves[__builtin_ctzll(m_board[piece_t::black_king])]) != 0)
    {
        return true;
    }
    if (((m_board[piece_t::white_king] &
          move_masks::king_moves[__builtin_ctzll(m_board[piece_t::black_king])])) != 0)
    {
        return true;
    }
    if (((((m_board[piece_t::black_king] >> 9) & ~masks::file_A) |
          ((m_board[piece_t::black_king] >> 7) & ~masks::file_H)) &
         m_board[piece_t::white_pawn]) != 0)
    {
        return true;
    }
    return false;
}
// NOLINTEND

auto MoveGen::compare_moves(const Move &mov_a, const Move &mov_b) -> bool
{
    // First compare move types
    if (mov_a.type != mov_b.type)
    {
        return mov_a.type > mov_b.type; // Higher type comes first
    }

    // If move types are the same, compare based on move type
    switch (mov_a.type)
    {
    case movType::QUIET:
        return mov_a.pc1 > mov_b.pc1; // Higher pc2 comes first

    case movType::CAPTURE:
        // Primary: compare captured pieces (pc2)
        if (mov_b.pc2 != mov_a.pc2)
        {
            return mov_a.pc2 > mov_b.pc2; // Higher pc2 comes first
        }
        // Secondary: compare capturing pieces (pc1)
        return mov_b.pc1 > mov_a.pc1; // Lower pc1 comes first

    case movType::PROMOTE:
        return mov_a.pc2 > mov_b.pc2; // Higher promotion piece comes first

    case movType::CAPTURE_PROMOTE:
        // Primary: compare promotion piece (pc3)
        if (mov_b.pc3 != mov_a.pc3)
        {
            return mov_a.pc3 > mov_b.pc3; // Higher pc3 comes first
        }
        // Secondary: compare captured pieces (pc2)
        return mov_a.pc2 > mov_b.pc2; // Higher pc2 comes first

    default:
        return false; // Equal (maintains stable sort)
    }
}

auto MoveGen::get(size_t idx) -> Move & { return m_movs[idx]; }

template<side_t side>
void MoveGen::gen()
{
    if constexpr (side == side_t::white)
    {
        get_white_queen_moves();
        get_white_rook_moves();
        get_white_bishop_moves();
        get_white_knight_moves();
        get_white_pawn_moves();
        get_white_king_moves();
    }
    else
    {
        get_black_queen_moves();
        get_black_rook_moves();
        get_black_bishop_moves();
        get_black_knight_moves();
        get_black_pawn_moves();
        get_black_king_moves();
    }

    m_end_idx = static_cast<ptrdiff_t>(m_idx);
    sort(m_movs.begin(), m_movs.begin() + m_end_idx, MoveGen::compare_moves);
}

MoveGen::MoveGen(const BitBoard &board) : m_board(board) {}

template void MoveGen::gen<side_t::white>();
template void MoveGen::gen<side_t::black>();