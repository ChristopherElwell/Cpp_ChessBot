#include "move_gen.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <print>

#include "bitboard.h"
#include "bitboard_constants.h"
#include "bitscan.h"
#include "data.h"
#include "move.h"

using namespace std;

namespace
{
template <int Shift>
[[nodiscard]] constexpr auto shift(const uint64_t bits) -> uint64_t
{
    if constexpr (Shift >= 0)
    {
        return bits << Shift;
    }
    else
    {
        return bits >> -Shift;
    }
}

[[nodiscard]] auto shift(const uint64_t bits, int shift) -> uint64_t
{
    if (shift >= 0)
    {
        return bits << shift;
    }
    return bits >> -shift;
}

[[nodiscard]] constexpr auto piece_kind(piece_t piece) -> int
{
    return static_cast<int>(piece) % 6;
}

constexpr int type_bucket = 64;

constexpr std::array<int, static_cast<size_t>(move_type_t::moves_termination)> type_priority = {
    /* quiet                  */ 0,
    /* capture                */ 6,
    /* promote_queen          */ 8,
    /* promote_rook           */ 5,
    /* promote_bishop         */ 4,
    /* promote_knight         */ 4,
    /* capture_promote_queen  */ 9,
    /* capture_promote_rook   */ 7,
    /* capture_promote_bishop */ 7,
    /* capture_promote_knight */ 7,
    /* castle_kingside        */ 3,
    /* castle_queenside       */ 3,
    /* pawn_double             */ 1,
    /* en_passent              */ 6,
};
}  // namespace

template <side_t Side>
auto MoveGen::get_rook_attacks(const uint64_t rook) const -> uint64_t
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

    return attacks & ~m_board[piece::all<Side>];
}

template <side_t Side>
auto MoveGen::get_bishop_attacks(const uint64_t bishop) const -> uint64_t
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
    spots = ((first_pc - 1) & sqs_ahead & up_ray) | (first_pc & m_board[piece::all<~Side>]);

    temp = pcs_behind & up_ray;
    mask = static_cast<int>(temp == 0) - 1;
    first_pc = (sq_a8 >> countl_zero(temp)) & mask;
    spots |= (bishop - 1) & ~((first_pc - 1) | first_pc) & up_ray;
    spots |= first_pc & m_board[piece::all<~Side>];
    spots |= up_ray & (bishop - 1) & ~mask;

    first_pc = pcs_ahead & -(pcs_ahead & down_ray) & down_ray;
    spots |= ((first_pc - 1) & sqs_ahead & down_ray) | (first_pc & m_board[piece::all<~Side>]);

    temp = pcs_behind & down_ray;
    mask = static_cast<int>(temp == 0) - 1;
    first_pc = (sq_a8 >> countl_zero(temp)) & mask;
    spots |= (bishop - 1) & ~((first_pc - 1) | first_pc) & down_ray;
    spots |= first_pc & m_board[piece::all<~Side>];
    spots |= down_ray & (bishop - 1) & ~mask;

    return spots;
}

template <side_t Side>
void MoveGen::get_knight_moves()
{
    for (const auto knight : bit_scan(m_board[piece::knight<Side>]))
    {
        const uint64_t moves =
            move_masks::knight_moves[countr_zero(knight)] & ~m_board[piece::all<Side>];

        add_to_movs<Side>(piece::knight<Side>, knight, moves);
    }
}

template <side_t Side>
void MoveGen::get_rook_moves()
{
    for (const auto rook : bit_scan(m_board[piece::rook<Side>]))
    {
        const uint64_t moves = get_rook_attacks<Side>(rook);
        add_to_movs<Side>(piece::rook<Side>, rook, moves);
    }
}

template <side_t Side>
void MoveGen::get_bishop_moves()
{
    for (const auto bishop : bit_scan(m_board[piece::bishop<Side>]))
    {
        const uint64_t moves = get_bishop_attacks<Side>(bishop);

        add_to_movs<Side>(piece::bishop<Side>, bishop, moves);
    }
}

template <side_t Side>
void MoveGen::get_pawn_moves()
{
    const uint64_t pawns = m_board[piece::pawn<Side>];

    const uint64_t one_step = shift<side_traits<Side>::pawn_push_dir>(pawns) &
                              ~side_traits<Side>::rank_8 & ~m_board[piece_t::all_pcs];

    for (const auto move : bit_scan(one_step))
    {
        m_movs[m_idx].move =
            Move(shift<-side_traits<Side>::pawn_push_dir>(move), move, move_type_t::quiet);
        m_movs[m_idx++].score = score_move(move_type_t::quiet, piece::pawn<Side>);
    }

    const uint64_t one_step_prom = shift<side_traits<Side>::pawn_push_dir>(pawns) &
                                   side_traits<Side>::rank_8 & ~m_board[piece_t::all_pcs];

    for (const auto move : bit_scan(one_step_prom))
    {
        const uint64_t from = shift<-side_traits<Side>::pawn_push_dir>(move);

        m_movs[m_idx].move = Move(from, move, move_type_t::promote_queen);
        m_movs[m_idx++].score = score_move(move_type_t::promote_queen, piece::pawn<Side>);
        m_movs[m_idx].move = Move(from, move, move_type_t::promote_rook);
        m_movs[m_idx++].score = score_move(move_type_t::promote_rook, piece::pawn<Side>);
        m_movs[m_idx].move = Move(from, move, move_type_t::promote_bishop);
        m_movs[m_idx++].score = score_move(move_type_t::promote_bishop, piece::pawn<Side>);
        m_movs[m_idx].move = Move(from, move, move_type_t::promote_knight);
        m_movs[m_idx++].score = score_move(move_type_t::promote_knight, piece::pawn<Side>);
    }

    const uint64_t two_steps =
        shift<2 * side_traits<Side>::pawn_push_dir>(pawns & side_traits<Side>::rank_2) &
        ~(m_board[piece_t::all_pcs] |
          shift<side_traits<Side>::pawn_push_dir>(m_board[piece_t::all_pcs]));

    for (const auto move : bit_scan(two_steps))
    {
        m_movs[m_idx].move = Move(shift<-2 * side_traits<Side>::pawn_push_dir>(move), move,
                                  move_type_t::pawn_double);
        m_movs[m_idx++].score = score_move(move_type_t::pawn_double, piece::pawn<Side>);
    }

    pawn_taking_moves<Side>(side_traits<Side>::pawn_take_w);
    pawn_taking_moves<Side>(side_traits<Side>::pawn_take_e);

    const uint64_t en_passent_take_w =
        (shift<side_traits<Side>::pawn_take_w>(pawns) & m_board[piece_t::info] & ~masks::rank_1 &
         ~side_traits<Side>::pawn_take_w_wrap_mask);

    if (en_passent_take_w != 0)
    {
        m_movs[m_idx].move = Move(shift<-side_traits<Side>::pawn_take_w>(en_passent_take_w),
                                  en_passent_take_w, move_type_t::en_passent);
        m_movs[m_idx++].score =
            score_move(move_type_t::en_passent, piece::pawn<Side>, piece::pawn<~Side>);
    }

    const uint64_t en_passent_take_e =
        (shift<side_traits<Side>::pawn_take_e>(pawns) & m_board[piece_t::info] & ~masks::rank_1 &
         ~side_traits<Side>::pawn_take_e_wrap_mask);

    if (en_passent_take_e != 0)
    {
        m_movs[m_idx].move = Move(shift<-side_traits<Side>::pawn_take_e>(en_passent_take_e),
                                  en_passent_take_e, move_type_t::en_passent);
        m_movs[m_idx++].score =
            score_move(move_type_t::en_passent, piece::pawn<Side>, piece::pawn<~Side>);
    }
}

template <side_t Side>
void MoveGen::pawn_taking_moves(const int offset)
{
    const uint64_t file_mask = offset == side_traits<Side>::pawn_take_e
                                   ? side_traits<Side>::pawn_take_e_wrap_mask
                                   : side_traits<Side>::pawn_take_w_wrap_mask;

    const uint64_t takes =
        shift(m_board[piece::pawn<Side>], offset) & m_board[piece::all<~Side>] & ~file_mask;

    for (const auto take : bit_scan(takes))
    {
        const piece_t taken_pc = m_board.piece_at(take);
        if (taken_pc == piece_t::none)
        {
            continue;
        }

        const uint64_t promotion_sq = take & side_traits<Side>::rank_8;

        if (promotion_sq != 0)
        {
            const uint64_t from = shift(take, -offset);

            m_movs[m_idx].move = Move(from, promotion_sq, move_type_t::capture_promote_queen);
            m_movs[m_idx++].score =
                score_move(move_type_t::capture_promote_queen, piece::pawn<Side>, taken_pc);
            m_movs[m_idx].move = Move(from, promotion_sq, move_type_t::capture_promote_rook);
            m_movs[m_idx++].score =
                score_move(move_type_t::capture_promote_rook, piece::pawn<Side>, taken_pc);
            m_movs[m_idx].move = Move(from, promotion_sq, move_type_t::capture_promote_bishop);
            m_movs[m_idx++].score =
                score_move(move_type_t::capture_promote_bishop, piece::pawn<Side>, taken_pc);
            m_movs[m_idx].move = Move(from, promotion_sq, move_type_t::capture_promote_knight);
            m_movs[m_idx++].score =
                score_move(move_type_t::capture_promote_knight, piece::pawn<Side>, taken_pc);
        }
        else
        {
            m_movs[m_idx].move = Move(shift(take, -offset), take, move_type_t::capture);
            m_movs[m_idx++].score = score_move(move_type_t::capture, piece::pawn<Side>, taken_pc);
        }
    }
}

template <side_t Side>
void MoveGen::get_king_moves()
{
    const uint64_t king = m_board[piece::king<Side>];

    const uint64_t moves =
        move_masks::king_moves.at(countr_zero(king)) & ~m_board[piece::all<Side>];

    add_to_movs<Side>(piece::king<Side>, king, moves);

    uint64_t attacks = 0;

    const bool has_kingside_rights = (m_board[piece_t::info] & castling<Side>::kingside_right) != 0;

    const bool has_kingside_space =
        (castling<Side>::kingside_space & m_board[piece_t::all_pcs]) == 0;

    const bool has_kingside_rook =
        (m_board[piece::rook<Side>] & side_traits<Side>::kingside_rook) != 0;

    const bool can_castle_kingside = has_kingside_rights && has_kingside_space && has_kingside_rook;

    if (can_castle_kingside)
    {
        attacks = get_attackers<~Side>();

        const bool kingside_path_is_safe = (attacks & castling<Side>::kingside_attacked) == 0;

        if (kingside_path_is_safe)
        {
            m_movs[m_idx].move =
                Move(castling<Side>::kingside_king_from, castling<Side>::kingside_king_to,
                     move_type_t::castle_kingside);
            m_movs[m_idx++].score = score_move(move_type_t::castle_kingside, piece::king<Side>);
        }
    }

    const bool has_queenside_rights =
        (m_board[piece_t::info] & castling<Side>::queenside_right) != 0;

    const bool has_queenside_space =
        (castling<Side>::queenside_space &
         (m_board[piece::all<Side>] | m_board[piece::all<~Side>])) == 0;

    const bool has_queenside_rook =
        (m_board[piece::rook<Side>] & side_traits<Side>::queenside_rook) != 0;

    const bool can_castle_queenside =
        has_queenside_rights && has_queenside_space && has_queenside_rook;

    if (can_castle_queenside)
    {
        if (attacks == 0)
        {
            attacks = get_attackers<~Side>();
        }

        const bool queenside_path_is_safe = (attacks & castling<Side>::queenside_attacked) == 0;

        if (queenside_path_is_safe)
        {
            m_movs[m_idx].move =
                Move(castling<Side>::queenside_king_from, castling<Side>::queenside_king_to,
                     move_type_t::castle_queenside);
            m_movs[m_idx++].score = score_move(move_type_t::castle_queenside, piece::king<Side>);
        }
    }
}

template <side_t Side>
void MoveGen::get_queen_moves()
{
    for (const auto queen : bit_scan(m_board[piece::queen<Side>]))
    {
        const uint64_t moves = get_bishop_attacks<Side>(queen) | get_rook_attacks<Side>(queen);

        add_to_movs<Side>(piece::queen<Side>, queen, moves);
    }
}

template <side_t Side>
void MoveGen::add_to_movs(const piece_t moving_pc, const uint64_t moving_pc_spot,
                          const uint64_t moves)
{
    for (const auto mov : bit_scan(moves & ~m_board[piece::all<~Side>]))
    {
        m_movs[m_idx].move = Move(moving_pc_spot, mov, move_type_t::quiet);
        m_movs[m_idx++].score = score_move(move_type_t::quiet, moving_pc);
    }

    for (const auto taking_spot : bit_scan(moves & m_board[piece::all<~Side>]))
    {
        const piece_t taken_pc = m_board.piece_at(taking_spot);
        if (taken_pc == piece_t::none)
        {
            continue;
        }
        m_movs[m_idx].move = Move(moving_pc_spot, taking_spot, move_type_t::capture);
        m_movs[m_idx++].score = score_move(move_type_t::capture, moving_pc, taken_pc);
    }
}

template <side_t Side>
auto MoveGen::get_attackers() -> uint64_t
{
    uint64_t attacks = 0;

    attacks |= shift<side_traits<Side>::pawn_take_w>(m_board[piece::pawn<Side>] &
                                                     ~side_traits<Side>::pawn_take_w_wrap_mask);

    attacks |= shift<side_traits<Side>::pawn_take_e>(m_board[piece::pawn<Side>] &
                                                     ~side_traits<Side>::pawn_take_e_wrap_mask);

    attacks |= move_masks::king_moves.at(countr_zero(m_board[piece::king<Side>]));

    for (const auto piece : bit_scan(m_board[piece::knight<Side>]))
    {
        attacks |= move_masks::knight_moves.at(countr_zero(piece));
    }

    for (const auto piece : bit_scan(m_board[piece::bishop<Side>] | m_board[piece::queen<Side>]))
    {
        attacks |= get_bishop_attacks<Side>(piece);
    }

    for (const auto piece : bit_scan(m_board[piece::rook<Side>] | m_board[piece::queen<Side>]))
    {
        attacks |= get_rook_attacks<Side>(piece);
    }

    return attacks;
}

template <side_t Side>
auto MoveGen::is_king_in_check() const -> bool
{
    const uint64_t king = m_board[piece::king<Side>];

    const bool attacked_by_rook_or_queen =
        ((m_board[piece::rook<~Side>] | m_board[piece::queen<~Side>]) &
         get_rook_attacks<Side>(king)) != 0;

    if (attacked_by_rook_or_queen)
    {
        return true;
    }

    const bool attacked_by_bishop_or_queen =
        ((m_board[piece::bishop<~Side>] | m_board[piece::queen<~Side>]) &
         get_bishop_attacks<Side>(king)) != 0;

    if (attacked_by_bishop_or_queen)
    {
        return true;
    }

    const bool attacked_by_knight =
        (m_board[piece::knight<~Side>] & move_masks::knight_moves.at(countr_zero(king))) != 0;

    if (attacked_by_knight)
    {
        return true;
    }

    const bool attacked_by_king =
        (m_board[piece::king<~Side>] & move_masks::king_moves.at(countr_zero(king))) != 0;

    if (attacked_by_king)
    {
        return true;
    }

    const uint64_t pawn_attacks_w =
        shift<side_traits<Side>::pawn_take_w>(king) & ~side_traits<Side>::pawn_take_w_wrap_mask;

    const uint64_t pawn_attacks_e =
        shift<side_traits<Side>::pawn_take_e>(king) & ~side_traits<Side>::pawn_take_e_wrap_mask;

    const bool attacked_by_pawn =
        ((pawn_attacks_w | pawn_attacks_e) & m_board[piece::pawn<~Side>]) != 0;

    return attacked_by_pawn;
}

auto MoveGen::score_move(move_type_t type, piece_t moving_pc, piece_t capturing_pc) -> int
{
    int score = type_priority[static_cast<size_t>(type)] * type_bucket;

    switch (type)
    {
        case move_type_t::quiet:
        case move_type_t::pawn_double:
        case move_type_t::castle_kingside:
        case move_type_t::castle_queenside:
            // Higher-kind piece moving scores slightly higher; mostly a
            // stable tiebreak, no capture involved.
            score += piece_kind(moving_pc);
            break;

        case move_type_t::capture:
        case move_type_t::en_passent:
            // MVV-LVA: most valuable victim first, least valuable attacker
            // as tiebreak.
            score += (piece_kind(capturing_pc) * 8) - piece_kind(moving_pc);
            break;

        case move_type_t::promote_queen:
        case move_type_t::promote_rook:
        case move_type_t::promote_bishop:
        case move_type_t::promote_knight:
            // Promotion piece is already encoded by `type`'s bucket;
            // nothing further to break ties on (moving_pc is always a pawn).
            break;

        case move_type_t::capture_promote_queen:
        case move_type_t::capture_promote_rook:
        case move_type_t::capture_promote_bishop:
        case move_type_t::capture_promote_knight:
            // Promotion piece is encoded by `type`'s bucket; tiebreak on
            // the captured piece (MVV).
            score += piece_kind(capturing_pc);
            break;

        default:
            break;
    }

    return score;
}

auto MoveGen::at(size_t idx) -> Move & { return m_movs[idx].move; }

auto MoveGen::at(size_t idx) const -> const Move & { return m_movs[idx].move; }

template <side_t Side>
void MoveGen::gen()
{
    get_pawn_moves<Side>();
    get_knight_moves<Side>();
    get_bishop_moves<Side>();
    get_rook_moves<Side>();
    get_queen_moves<Side>();
    get_king_moves<Side>();

    m_end_idx = static_cast<ptrdiff_t>(m_idx);
    std::sort(m_movs.begin(), m_movs.begin() + m_end_idx,
              [](const scored_move &move_a, const scored_move &move_b) -> bool
              {
                  return move_a.score > move_b.score;  // descending: best score first
              });
}

MoveGen::MoveGen(const BitBoard &board) : m_board(board) {}

template void MoveGen::gen<side_t::white>();
template void MoveGen::gen<side_t::black>();

template bool MoveGen::is_king_in_check<side_t::white>() const;
template bool MoveGen::is_king_in_check<side_t::black>() const;

template uint64_t MoveGen::get_attackers<side_t::white>();
template uint64_t MoveGen::get_attackers<side_t::black>();

template void MoveGen::add_to_movs<side_t::white>(piece_t, uint64_t, uint64_t);
template void MoveGen::add_to_movs<side_t::black>(piece_t, uint64_t, uint64_t);

template uint64_t MoveGen::get_rook_attacks<side_t::white>(uint64_t) const;
template uint64_t MoveGen::get_rook_attacks<side_t::black>(uint64_t) const;

template uint64_t MoveGen::get_bishop_attacks<side_t::white>(uint64_t) const;
template uint64_t MoveGen::get_bishop_attacks<side_t::black>(uint64_t) const;

template void MoveGen::get_pawn_moves<side_t::white>();
template void MoveGen::get_pawn_moves<side_t::black>();

template void MoveGen::pawn_taking_moves<side_t::white>(int);
template void MoveGen::pawn_taking_moves<side_t::black>(int);

template void MoveGen::get_knight_moves<side_t::white>();
template void MoveGen::get_knight_moves<side_t::black>();

template void MoveGen::get_bishop_moves<side_t::white>();
template void MoveGen::get_bishop_moves<side_t::black>();

template void MoveGen::get_rook_moves<side_t::white>();
template void MoveGen::get_rook_moves<side_t::black>();

template void MoveGen::get_queen_moves<side_t::white>();
template void MoveGen::get_queen_moves<side_t::black>();

template void MoveGen::get_king_moves<side_t::white>();
template void MoveGen::get_king_moves<side_t::black>();
