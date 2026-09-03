#include "move_gen.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <print>

#include "bitboard.h"
#include "bitscan.h"
#include "data.h"
#include "move.h"

using namespace std;

auto MoveGen::get_white_rook_attacks(const uint64_t rook) const -> uint64_t
{
    const int pos = countr_zero(rook);
    const int rank = pos >> 3;
    const int file = pos & 7;
    const int base = pos & ~7;
    uint64_t attacks =
        (uint64_t)(move_masks::sliding_moves[(((m_board[piece_t::all_pcs] >> (base + 1)) &
                                               move_masks::sliding_moves_mask)
                                              << 3) +
                                             file])
        << base;

    const uint64_t file_isolated = m_board[piece_t::all_pcs] << (8 - file) & masks::file_h;
    const uint64_t rotated = (file_isolated * masks::anti_diag) >> 56;
    const uint64_t index = ((rotated * 8) + (7 - rank)) & 0x1ff;
    const uint64_t moves_rotated = ((uint64_t)move_masks::sliding_moves[index]) * masks::anti_diag;
    attacks |= (moves_rotated & masks::file_a) >> (7 - file);

    return attacks & ~m_board[piece_t::white_pcs];
}

auto MoveGen::get_black_rook_attacks(const uint64_t rook) const -> uint64_t
{
    const int pos = countr_zero(rook);
    const int rank = pos >> 3;
    const int file = pos & 7;
    const int base = pos & ~7;

    uint64_t attacks =
        (uint64_t)(move_masks::sliding_moves[(((m_board[piece_t::all_pcs] >> (base + 1)) &
                                               move_masks::sliding_moves_mask)
                                              << 3) +
                                             file])
        << base;

    const uint64_t file_isolated = m_board[piece_t::all_pcs] << (8 - file) & masks::file_h;
    const uint64_t rotated = (file_isolated * masks::anti_diag) >> 56;
    const uint64_t index = ((rotated * 8) + (7 - rank)) & 0x1ff;
    const uint64_t moves_rotated = ((uint64_t)move_masks::sliding_moves[index]) * masks::anti_diag;
    attacks |= (moves_rotated & masks::file_a) >> (7 - file);

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
    const int pos = countr_zero(bishop);
    up_ray = masks::diag_up[(pos & 7) + (pos >> 3)];
    down_ray = masks::diag_down[7 + (pos >> 3) - (pos & 7)];

    const uint64_t sqs_ahead = ~((bishop - 1) | bishop);
    const uint64_t pcs_ahead = m_board[piece_t::all_pcs] & sqs_ahead;
    const uint64_t pcs_behind = m_board[piece_t::all_pcs] & (bishop - 1);

    first_pc = pcs_ahead & -(pcs_ahead & up_ray) & up_ray;
    spots = ((first_pc - 1) & sqs_ahead & up_ray) | (first_pc & m_board[piece_t::black_pcs]);

    temp = pcs_behind & up_ray;
    mask = static_cast<int>(temp == 0) - 1;
    first_pc = (sq_a8 >> countl_zero(temp)) & mask;
    spots |= (bishop - 1) & ~((first_pc - 1) | first_pc) & up_ray;
    spots |= first_pc & m_board[piece_t::black_pcs];
    spots |= up_ray & (bishop - 1) & ~mask;

    first_pc = pcs_ahead & -(pcs_ahead & down_ray) & down_ray;
    spots |= ((first_pc - 1) & sqs_ahead & down_ray) | (first_pc & m_board[piece_t::black_pcs]);

    temp = pcs_behind & down_ray;
    mask = static_cast<int>(temp == 0) - 1;
    first_pc = (sq_a8 >> countl_zero(temp)) & mask;
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
    const int pos = countr_zero(bishop);
    up_ray = masks::diag_up[(pos & 7) + (pos >> 3)];
    down_ray = masks::diag_down[7 + (pos >> 3) - (pos & 7)];

    const uint64_t sqs_ahead = ~((bishop - 1) | bishop);
    const uint64_t pcs_ahead = m_board[piece_t::all_pcs] & sqs_ahead;
    const uint64_t pcs_behind = m_board[piece_t::all_pcs] & (bishop - 1);

    first_pc = pcs_ahead & -(pcs_ahead & up_ray) & up_ray;
    spots = ((first_pc - 1) & sqs_ahead & up_ray) | (first_pc & m_board[piece_t::white_pcs]);

    temp = pcs_behind & up_ray;
    mask = static_cast<int>(temp == 0) - 1;
    first_pc = (sq_a8 >> countl_zero(temp)) & mask;
    spots |= (bishop - 1) & ~((first_pc - 1) | first_pc) & up_ray;
    spots |= first_pc & m_board[piece_t::white_pcs];
    spots |= up_ray & (bishop - 1) & ~mask;

    first_pc = pcs_ahead & -(pcs_ahead & down_ray) & down_ray;
    spots |= ((first_pc - 1) & sqs_ahead & down_ray) | (first_pc & m_board[piece_t::white_pcs]);

    temp = pcs_behind & down_ray;
    mask = static_cast<int>(temp == 0) - 1;
    first_pc = (sq_a8 >> countl_zero(temp)) & mask;
    spots |= (bishop - 1) & ~((first_pc - 1) | first_pc) & down_ray;
    spots |= first_pc & m_board[piece_t::white_pcs];
    spots |= down_ray & (bishop - 1) & ~mask;
    return spots;
}

void MoveGen::get_white_knight_moves()
{
    for (const auto knight : bit_scan(m_board[piece_t::white_knight]))
    {
        const uint64_t moves =
            move_masks::knight_moves[countr_zero(knight)] & ~m_board[piece_t::white_pcs];
        white_add_to_movs(knight, moves);
    }
}

void MoveGen::get_black_knight_moves()
{
    for (const auto knight : bit_scan(m_board[piece_t::black_knight]))
    {
        const uint64_t moves =
            move_masks::knight_moves[countr_zero(knight)] & ~m_board[piece_t::black_pcs];
        black_add_to_movs(knight, moves);
    }
}

void MoveGen::get_white_rook_moves()
{
    for (const auto rook : bit_scan(m_board[piece_t::white_rook]))
    {
        const uint64_t moves = get_white_rook_attacks(rook);
        white_add_to_movs(rook, moves);
    }
}

void MoveGen::get_black_rook_moves()
{
    for (const auto rook : bit_scan(m_board[piece_t::black_rook]))
    {
        const uint64_t moves = get_black_rook_attacks(rook);
        black_add_to_movs(rook, moves);
    }
}

void MoveGen::get_white_bishop_moves()
{
    for (const auto bishop : bit_scan(m_board[piece_t::white_bishop]))
    {
        const uint64_t moves = get_white_bishop_attacks(bishop);

        white_add_to_movs(bishop, moves);
    }
}

void MoveGen::get_black_bishop_moves()
{
    for (const auto bishop : bit_scan(m_board[piece_t::black_bishop]))
    {
        const uint64_t moves = get_black_bishop_attacks(bishop);

        black_add_to_movs(bishop, moves);
    }
}

void MoveGen::get_white_pawn_moves()
{
    for (const auto one_step : bit_scan((m_board[piece_t::white_pawn] << 8) & ~masks::rank_8 &
                                        ~m_board[piece_t::all_pcs]))
    {
        m_movs[m_idx++] = Move(one_step >> 8, one_step, move_type_t::quiet);
    }
    for (const auto one_step_prom :
         bit_scan((m_board[piece_t::white_pawn] << 8) & masks::rank_8 & ~m_board[piece_t::all_pcs]))
    {
        m_movs[m_idx++] = Move(one_step_prom >> 8, one_step_prom, move_type_t::promote_queen);
        m_movs[m_idx++] = Move(one_step_prom >> 8, one_step_prom, move_type_t::promote_rook);
        m_movs[m_idx++] = Move(one_step_prom >> 8, one_step_prom, move_type_t::promote_bishop);
        m_movs[m_idx++] = Move(one_step_prom >> 8, one_step_prom, move_type_t::promote_knight);
    }

    const uint64_t two_steps = ((m_board[piece_t::white_pawn] & masks::rank_2) << 16) &
                               ~((m_board[piece_t::all_pcs]) | (m_board[piece_t::all_pcs] << 8));
    for (const auto two_step : bit_scan(two_steps))
    {
        m_movs[m_idx++] = Move(two_step >> 16, two_step, move_type_t::pawn_double);
    }

    white_pawn_taking_moves(7);
    white_pawn_taking_moves(9);

    const uint64_t en_passent_take_left =
        ((m_board[piece_t::white_pawn] << 9) & m_board[piece_t::info] & ~masks::rank_1 &
         ~masks::file_h);
    if (en_passent_take_left != 0)
    {
        m_movs[m_idx++] =
            Move(en_passent_take_left >> 9, en_passent_take_left, move_type_t::en_passent);
    }

    const uint64_t en_passent_take_right =
        ((m_board[piece_t::white_pawn] << 7) & m_board[piece_t::info] & ~masks::rank_1 &
         ~masks::file_a);
    if (en_passent_take_right != 0)
    {
        m_movs[m_idx++] =
            Move((en_passent_take_right >> 7), en_passent_take_right, move_type_t::en_passent);
    }
}

void MoveGen::get_black_pawn_moves()
{
    for (const auto one_step : bit_scan((m_board[piece_t::black_pawn] >> 8) & ~masks::rank_1 &
                                        ~m_board[piece_t::all_pcs]))
    {
        m_movs[m_idx++] = Move(one_step << 8, one_step, move_type_t::quiet);
    }

    for (const auto one_step_prom :
         bit_scan((m_board[piece_t::black_pawn] >> 8) & masks::rank_1 & ~m_board[piece_t::all_pcs]))
    {
        m_movs[m_idx++] = Move(one_step_prom << 8, one_step_prom, move_type_t::promote_queen);
        m_movs[m_idx++] = Move(one_step_prom << 8, one_step_prom, move_type_t::promote_rook);
        m_movs[m_idx++] = Move(one_step_prom << 8, one_step_prom, move_type_t::promote_bishop);
        m_movs[m_idx++] = Move(one_step_prom << 8, one_step_prom, move_type_t::promote_knight);
    }

    const uint64_t two_steps = ((m_board[piece_t::black_pawn] & masks::rank_7) >> 16) &
                               ~((m_board[piece_t::all_pcs]) | (m_board[piece_t::all_pcs] >> 8));
    for (const auto two_step : bit_scan(two_steps))
    {
        m_movs[m_idx++] = Move(two_step << 16, two_step, move_type_t::pawn_double);
    }

    black_pawn_taking_moves(9);
    black_pawn_taking_moves(7);
    const uint64_t en_passent_take_left =
        ((m_board[piece_t::black_pawn] >> 7) & m_board[piece_t::info] & ~masks::rank_1 &
         ~masks::file_h);
    if (en_passent_take_left != 0)
    {
        m_movs[m_idx++] =
            Move((en_passent_take_left << 7), en_passent_take_left, move_type_t::en_passent);
    }

    const uint64_t en_passent_take_right =
        ((m_board[piece_t::black_pawn] >> 9) & m_board[piece_t::info] & ~masks::rank_1 &
         ~masks::file_a);
    if (en_passent_take_right != 0)
    {
        m_movs[m_idx++] =
            Move((en_passent_take_right << 9), en_passent_take_right, move_type_t::en_passent);
    }
}

void MoveGen::black_pawn_taking_moves(const int offset)
{
    uint64_t const file_mask = offset == 7 ? masks::file_h : masks::file_a;
    for (const auto take : bit_scan((m_board[piece_t::black_pawn] >> offset) &
                                    m_board[piece_t::white_pcs] & ~file_mask))
    {
        for (auto const piece : piece_range::white_no_king())
        {
            const uint64_t taken_piece = (take & m_board[piece]);
            if (taken_piece == 0)
            {
                continue;
            }
            const uint64_t promotion_sq = (take & masks::rank_1);
            if (promotion_sq != 0)
            {
                m_movs[m_idx++] =
                    Move(take << offset, promotion_sq, move_type_t::capture_promote_queen);
                m_movs[m_idx++] =
                    Move(take << offset, promotion_sq, move_type_t::capture_promote_rook);
                m_movs[m_idx++] =
                    Move(take << offset, promotion_sq, move_type_t::capture_promote_bishop);
                m_movs[m_idx++] =
                    Move(take << offset, promotion_sq, move_type_t::capture_promote_knight);
            }
            else
            {
                m_movs[m_idx++] = Move(take << offset, take, move_type_t::capture);
            }
            break;
        }
    }
}

void MoveGen::white_pawn_taking_moves(const int offset)
{
    uint64_t const file_mask = offset == 7 ? masks::file_a : masks::file_h;
    for (const auto take : bit_scan((m_board[piece_t::white_pawn] << offset) &
                                    m_board[piece_t::black_pcs] & ~file_mask))
    {
        for (auto const piece : piece_range::black_no_king())
        {
            const uint64_t taken_piece = (take & m_board[piece]);
            if (taken_piece == 0)
            {
                continue;
            }
            const uint64_t promotion_sq = (take & masks::rank_8);
            if (promotion_sq != 0)
            {
                m_movs[m_idx++] =
                    Move(take >> offset, promotion_sq, move_type_t::capture_promote_queen);
                m_movs[m_idx++] =
                    Move(take >> offset, promotion_sq, move_type_t::capture_promote_rook);
                m_movs[m_idx++] =
                    Move(take >> offset, promotion_sq, move_type_t::capture_promote_bishop);
                m_movs[m_idx++] =
                    Move(take >> offset, promotion_sq, move_type_t::capture_promote_knight);
            }
            else
            {
                m_movs[m_idx++] = Move(take >> offset, take, move_type_t::capture);
            }
            break;
        }
    }
}

void MoveGen::get_white_king_moves()
{
    const uint64_t moves = move_masks::king_moves.at(countr_zero(m_board[piece_t::white_king])) &
                           ~m_board[piece_t::white_pcs];

    white_add_to_movs(m_board[piece_t::white_king], moves);

    uint64_t attacks = 0;
    if (((m_board[piece_t::info] & castling<side_t::white>::kingside_right) != 0) &&
        ((castling<side_t::white>::kingside_space &
          (m_board[piece_t::white_pcs] | m_board[piece_t::black_pcs])) == 0) &&
        ((m_board[piece_t::white_rook] & masks::file_h & masks::rank_1) != 0))
    {
        attacks = get_black_attackers(m_board);
        if ((attacks & castling<side_t::white>::kingside_attacked) == 0)
        {
            m_movs[m_idx++] =
                Move(castling<side_t::white>::kingside_king_from,
                     castling<side_t::white>::kingside_king_to, move_type_t::castle_kingside);
        }
    }
    if (((m_board[piece_t::info] & castling<side_t::white>::queenside_right) != 0) &&
        ((castling<side_t::white>::queenside_space &
          (m_board[piece_t::white_pcs] | m_board[piece_t::black_pcs])) == 0) &&
        ((m_board[piece_t::white_rook] & masks::file_a & masks::rank_1) != 0))
    {
        if (attacks == 0)
        {
            attacks = get_black_attackers(m_board);
        }

        if ((attacks & castling<side_t::white>::queenside_attacked) == 0)
        {
            m_movs[m_idx++] =
                Move(castling<side_t::white>::queenside_king_from,
                     castling<side_t::white>::queenside_king_to, move_type_t::castle_queenside);
        }
    }
}

void MoveGen::get_black_king_moves()
{
    const uint64_t moves = move_masks::king_moves.at(countr_zero(m_board[piece_t::black_king])) &
                           ~m_board[piece_t::black_pcs];

    black_add_to_movs(m_board[piece_t::black_king], moves);

    uint64_t attacks = 0;
    if (((m_board[piece_t::info] & castling<side_t::black>::kingside_right) != 0) &&
        ((castling<side_t::black>::kingside_space &
          (m_board[piece_t::white_pcs] | m_board[piece_t::black_pcs])) == 0) &&
        ((m_board[piece_t::black_rook] & masks::file_h & masks::rank_8) != 0))
    {
        attacks = get_white_attackers(m_board);
        if ((attacks & castling<side_t::black>::kingside_attacked) == 0)
        {
            m_movs[m_idx++] =
                Move(castling<side_t::black>::kingside_king_from,
                     castling<side_t::black>::kingside_king_to, move_type_t::castle_kingside);
        }
    }

    if (((m_board[piece_t::info] & castling<side_t::black>::queenside_right) != 0) &&
        ((castling<side_t::black>::queenside_space &
          (m_board[piece_t::white_pcs] | m_board[piece_t::black_pcs])) == 0) &&
        ((m_board[piece_t::black_rook] & masks::file_a & masks::rank_8) != 0))
    {
        if (attacks == 0)
        {
            attacks = get_white_attackers(m_board);
        }

        if ((attacks & castling<side_t::black>::queenside_attacked) == 0)
        {
            m_movs[m_idx++] =
                Move(castling<side_t::black>::queenside_king_from,
                     castling<side_t::black>::queenside_king_to, move_type_t::castle_queenside);
        }
    }
}

void MoveGen::get_white_queen_moves()
{
    for (const auto queen : bit_scan(m_board[piece_t::white_queen]))
    {
        const uint64_t moves = get_white_bishop_attacks(queen) | get_white_rook_attacks(queen);

        white_add_to_movs(queen, moves);
    }
}

void MoveGen::get_black_queen_moves()
{
    for (const auto queen : bit_scan(m_board[piece_t::black_queen]))
    {
        const uint64_t moves = get_black_bishop_attacks(queen) | get_black_rook_attacks(queen);

        black_add_to_movs(queen, moves);
    }
}

void MoveGen::white_add_to_movs(const uint64_t moving_pc_spot, const uint64_t moves)
{
    for (const auto mov : bit_scan(moves & ~m_board[piece_t::black_pcs]))
    {
        m_movs[m_idx++] = Move(moving_pc_spot, mov, move_type_t::quiet);
    }

    for (const auto taking_spot : bit_scan(moves & m_board[piece_t::black_pcs]))
    {
        for (const auto taken_pc : piece_range::black_no_king())
        {
            const uint64_t taken_spot = (taking_spot & m_board[taken_pc]);
            if (taken_spot != 0)
            {
                m_movs[m_idx++] = Move(moving_pc_spot, taken_spot, move_type_t::capture);
                break;
            }
        }
    }
}

void MoveGen::black_add_to_movs(const uint64_t moving_pc_spot, const uint64_t moves)
{
    for (const auto mov : bit_scan(moves & ~m_board[piece_t::white_pcs]))
    {
        m_movs[m_idx++] = Move(moving_pc_spot, mov, move_type_t::quiet);
    }

    for (const auto taking_spot : bit_scan(moves & m_board[piece_t::white_pcs]))
    {
        for (const auto taken_pc : piece_range::white_no_king())
        {
            const uint64_t taken_spot = (taking_spot & m_board[taken_pc]);
            if (taken_spot != 0)
            {
                m_movs[m_idx++] = Move(moving_pc_spot, taken_spot, move_type_t::capture);
                break;
            }
        }
    }
}

auto MoveGen::get_white_attackers(const BitBoard &m_board) -> uint64_t
{
    uint64_t attacks = 0;
    attacks |= ((m_board[piece_t::white_pawn] & ~masks::file_h) << 7) |
               ((m_board[piece_t::white_pawn] & ~masks::file_a) << 9);

    attacks |= move_masks::king_moves.at(countr_zero(m_board[piece_t::white_king]));

    for (const auto piece : bit_scan(m_board[piece_t::white_knight]))
    {
        attacks |= move_masks::knight_moves.at(countr_zero(piece));
    }

    for (const auto piece :
         bit_scan((m_board[piece_t::white_bishop] | m_board[piece_t::white_queen])))
    {
        attacks |= get_white_bishop_attacks(piece);
    }

    for (const auto piece :
         bit_scan((m_board[piece_t::white_rook] | m_board[piece_t::white_queen])))
    {
        attacks |= get_white_rook_attacks(piece);
    }

    return attacks;
}

auto MoveGen::get_black_attackers(const BitBoard &m_board) -> uint64_t
{
    uint64_t attacks = 0;

    attacks |= ((m_board[piece_t::black_pawn] & ~masks::file_h) >> 9) |
               ((m_board[piece_t::black_pawn] & ~masks::file_a) >> 7);

    attacks |= move_masks::king_moves.at(countr_zero(m_board[piece_t::black_king]));

    for (const auto piece : bit_scan(m_board[piece_t::black_knight]))
    {
        attacks |= move_masks::knight_moves.at(countr_zero(piece));
    }

    for (const auto piece :
         bit_scan(m_board[piece_t::black_bishop] | m_board[piece_t::black_queen]))
    {
        attacks |= get_black_bishop_attacks(piece);
    }

    for (const auto piece : bit_scan(m_board[piece_t::black_rook] | m_board[piece_t::black_queen]))
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
         move_masks::knight_moves.at(countr_zero(m_board[piece_t::white_king]))) != 0)
    {
        return true;
    }
    if ((m_board[piece_t::black_king] &
         move_masks::king_moves.at(countr_zero(m_board[piece_t::white_king]))) != 0)
    {
        return true;
    }
    if (((((m_board[piece_t::white_king] << 9) & ~masks::file_h) |
          ((m_board[piece_t::white_king] << 7) & ~masks::file_a)) &
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
         move_masks::knight_moves.at(countr_zero(m_board[piece_t::black_king]))) != 0)
    {
        return true;
    }
    if (((m_board[piece_t::white_king] &
          move_masks::king_moves.at(countr_zero(m_board[piece_t::black_king])))) != 0)
    {
        return true;
    }
    if (((((m_board[piece_t::black_king] >> 9) & ~masks::file_a) |
          ((m_board[piece_t::black_king] >> 7) & ~masks::file_h)) &
         m_board[piece_t::white_pawn]) != 0)
    {
        return true;
    }
    return false;
}
// NOLINTEND

// auto MoveGen::compare_moves(const Move &mov_a, const Move &mov_b) -> bool
// {
//     const piece_t a_moved = m_board.piece_at(mov_a.from());
//     const piece_t a_captured = m_board.piece_at(mov_a.to());
//     // First compare move types
//     if (mov_a.type() != mov_b.type())
//     {
//         return mov_a.type() > mov_b.type();  // Higher type comes first
//     }
//
//     // If move types are the same, compare based on move type
//     switch (mov_a.type())
//     {
//         case move_type_t::quiet:
//             return mov_a.pc1 > mov_b.pc1;  // Higher pc2 comes first
//
//         case move_type_t::capture:
//             // Primary: compare captured pieces (pc2)
//             if (mov_b.pc2 != mov_a.pc2)
//             {
//                 return mov_a.pc2 > mov_b.pc2;  // Higher pc2 comes first
//             }
//             // Secondary: compare capturing pieces (pc1)
//             return mov_b.pc1 > mov_a.pc1;  // Lower pc1 comes first
//
//         case move_type_t::promote:
//             return mov_a.pc2 > mov_b.pc2;  // Higher promotion piece comes first
//
//         case move_type_t::capture_promote:
//             // Primary: compare promotion piece (pc3)
//             if (mov_b.pc3 != mov_a.pc3)
//             {
//                 return mov_a.pc3 > mov_b.pc3;  // Higher pc3 comes first
//             }
//             // Secondary: compare captured pieces (pc2)
//             return mov_a.pc2 > mov_b.pc2;  // Higher pc2 comes first
//
//         default:
//             return false;  // Equal (maintains stable sort)
//     }
// }

auto MoveGen::at(size_t idx) -> Move & { return m_movs[idx]; }
auto MoveGen::at(size_t idx) const -> const Move & { return m_movs[idx]; }

template <side_t Side>
void MoveGen::gen()
{
    if constexpr (Side == side_t::white)
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
    // sort(m_movs.begin(), m_movs.begin() + m_end_idx, MoveGen::compare_moves);
}

MoveGen::MoveGen(const BitBoard &board) : m_board(board) {}

template void MoveGen::gen<side_t::white>();
template void MoveGen::gen<side_t::black>();
