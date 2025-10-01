#include "move_gen.h"
#include "bitboard.h"
#include "bitscan.h"
#include "data.h"
#include "helpers.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <print>

using namespace std;

auto MoveGen::get_white_rook_attacks(const uint64_t rook) const -> uint64_t {
  const int pos = __builtin_ctzll(rook);
  const int rank = pos >> 3;
  const int file = pos & 7;
  const int base = pos & ~7;
  uint64_t attacks =
      (uint64_t)(data::SLIDING_MOVES
                     [(((m_board[Piece::ALL_PCS] >> (base + 1)) & 0x3F) << 3) +
                      file])
      << base;

  const uint64_t file_isolated =
      m_board[Piece::ALL_PCS] << (8 - file) & masks::FILE_H;
  const uint64_t rotated = (file_isolated * masks::ANTI_DIAG) >> 56;
  const uint64_t index = (rotated * 8 + (7 - rank)) & 0x1ff;
  const uint64_t moves_rotated =
      ((uint64_t)data::SLIDING_MOVES[index]) * masks::ANTI_DIAG;
  attacks |= (moves_rotated & masks::FILE_A) >> (7 - file);

  return attacks & ~m_board[Piece::WHITE_PCS];
}

auto MoveGen::get_black_rook_attacks(const uint64_t rook) const -> uint64_t {
  const int pos = __builtin_ctzll(rook);
  const int rank = pos >> 3;
  const int file = pos & 7;
  const int base = pos & ~7;

  uint64_t attacks =
      (uint64_t)(data::SLIDING_MOVES
                     [(((m_board[Piece::ALL_PCS] >> (base + 1)) & 0x3F) << 3) +
                      file])
      << base;

  const uint64_t file_isolated =
      m_board[Piece::ALL_PCS] << (8 - file) & masks::FILE_H;
  const uint64_t rotated = (file_isolated * masks::ANTI_DIAG) >> 56;
  const uint64_t index = (rotated * 8 + (7 - rank)) & 0x1ff;
  const uint64_t moves_rotated =
      ((uint64_t)data::SLIDING_MOVES[index]) * masks::ANTI_DIAG;
  attacks |= (moves_rotated & masks::FILE_A) >> (7 - file);

  return attacks & ~m_board[Piece::BLACK_PCS];
}

auto MoveGen::get_white_bishop_attacks(const uint64_t bishop) const
    -> uint64_t {

  uint64_t up_ray = 0;
  uint64_t down_ray = 0;
  uint64_t mask = 0;
  uint64_t temp = 0;
  uint64_t first_pc = 0;
  uint64_t spots = 0;
  const int pos = __builtin_ctzll(bishop);
  up_ray = masks::DIAGS_UP[(pos & 7) + (pos >> 3)];
  down_ray = masks::DIAGS_DOWN[7 + (pos >> 3) - (pos & 7)];

  const uint64_t sqs_ahead = ~((bishop - 1) | bishop);
  const uint64_t pcs_ahead = m_board[Piece::ALL_PCS] & sqs_ahead;
  const uint64_t pcs_behind = m_board[Piece::ALL_PCS] & (bishop - 1);

  first_pc = pcs_ahead & -(pcs_ahead & up_ray) & up_ray;
  spots = ((first_pc - 1) & sqs_ahead & up_ray) |
          (first_pc & m_board[Piece::BLACK_PCS]);

  temp = pcs_behind & up_ray;
  mask = static_cast<int>(temp == 0) - 1;
  first_pc = (sq_A8 >> __builtin_clzll(temp)) & mask;
  spots |= (bishop - 1) & ~((first_pc - 1) | first_pc) & up_ray;
  spots |= first_pc & m_board[Piece::BLACK_PCS];
  spots |= up_ray & (bishop - 1) & ~mask;

  first_pc = pcs_ahead & -(pcs_ahead & down_ray) & down_ray;
  spots |= ((first_pc - 1) & sqs_ahead & down_ray) |
           (first_pc & m_board[Piece::BLACK_PCS]);

  temp = pcs_behind & down_ray;
  mask = static_cast<int>(temp == 0) - 1;
  first_pc = (sq_A8 >> __builtin_clzll(temp)) & mask;
  spots |= (bishop - 1) & ~((first_pc - 1) | first_pc) & down_ray;
  spots |= first_pc & m_board[Piece::BLACK_PCS];
  spots |= down_ray & (bishop - 1) & ~mask;
  return spots;
}

auto MoveGen::get_black_bishop_attacks(const uint64_t bishop) const
    -> uint64_t {
  uint64_t up_ray = 0;
  uint64_t down_ray = 0;
  uint64_t mask = 0;
  uint64_t temp = 0;
  uint64_t first_pc = 0;
  uint64_t spots = 0;
  const int pos = __builtin_ctzll(bishop);
  up_ray = masks::DIAGS_UP[(pos & 7) + (pos >> 3)];
  down_ray = masks::DIAGS_DOWN[7 + (pos >> 3) - (pos & 7)];

  const uint64_t sqs_ahead = ~((bishop - 1) | bishop);
  const uint64_t pcs_ahead = m_board[Piece::ALL_PCS] & sqs_ahead;
  const uint64_t pcs_behind = m_board[Piece::ALL_PCS] & (bishop - 1);

  first_pc = pcs_ahead & -(pcs_ahead & up_ray) & up_ray;
  spots = ((first_pc - 1) & sqs_ahead & up_ray) |
          (first_pc & m_board[Piece::WHITE_PCS]);

  temp = pcs_behind & up_ray;
  mask = static_cast<int>(temp == 0) - 1;
  first_pc = (sq_A8 >> __builtin_clzll(temp)) & mask;
  spots |= (bishop - 1) & ~((first_pc - 1) | first_pc) & up_ray;
  spots |= first_pc & m_board[Piece::WHITE_PCS];
  spots |= up_ray & (bishop - 1) & ~mask;

  first_pc = pcs_ahead & -(pcs_ahead & down_ray) & down_ray;
  spots |= ((first_pc - 1) & sqs_ahead & down_ray) |
           (first_pc & m_board[Piece::WHITE_PCS]);

  temp = pcs_behind & down_ray;
  mask = static_cast<int>(temp == 0) - 1;
  first_pc = (sq_A8 >> __builtin_clzll(temp)) & mask;
  spots |= (bishop - 1) & ~((first_pc - 1) | first_pc) & down_ray;
  spots |= first_pc & m_board[Piece::WHITE_PCS];
  spots |= down_ray & (bishop - 1) & ~mask;
  return spots;
}

void MoveGen::get_white_knight_moves() {
  for (const auto knight : BitScan(m_board[Piece::WHITE_KNIGHT])) {
    const uint64_t moves = data::KNIGHT_MOVES[__builtin_ctzll(knight)] &
                           ~m_board[Piece::WHITE_PCS];
    white_add_to_movs(Piece::WHITE_KNIGHT, knight, moves, 0);
  }
}

void MoveGen::get_black_knight_moves() {
  for (const auto knight : BitScan(m_board[Piece::BLACK_KNIGHT])) {
    const uint64_t moves = data::KNIGHT_MOVES[__builtin_ctzll(knight)] &
                           ~m_board[Piece::BLACK_PCS];
    black_add_to_movs(Piece::BLACK_KNIGHT, knight, moves, 0);
  }
}

void MoveGen::get_white_rook_moves() {
  for (const auto rook : BitScan(m_board[Piece::WHITE_ROOK])) {
    const uint64_t moves = get_white_rook_attacks(rook);
    const uint64_t info =
        m_board[Piece::INFO] & rook &
        (castling::WHITE_KINGSIDE_RIGHT | castling::WHITE_QUEENSIDE_RIGHT);
    white_add_to_movs(Piece::WHITE_ROOK, rook, moves, info);
  }
}

void MoveGen::get_black_rook_moves() {
  for (const auto rook : BitScan(m_board[Piece::BLACK_ROOK])) {
    const uint64_t moves = get_black_rook_attacks(rook);
    const uint64_t info =
        m_board[Piece::INFO] & rook &
        (castling::BLACK_KINGSIDE_RIGHT | castling::BLACK_QUEENSIDE_RIGHT);
    black_add_to_movs(Piece::BLACK_ROOK, rook, moves, info);
  }
}

void MoveGen::get_white_bishop_moves() {
  for (const auto bishop : BitScan(m_board[Piece::WHITE_BISHOP])) {
    const uint64_t moves = get_white_bishop_attacks(bishop);

    white_add_to_movs(Piece::WHITE_BISHOP, bishop, moves, 0);
  }
}

void MoveGen::get_black_bishop_moves() {
  for (const auto bishop : BitScan(m_board[Piece::BLACK_BISHOP])) {
    const uint64_t moves = get_black_bishop_attacks(bishop);

    black_add_to_movs(Piece::BLACK_BISHOP, bishop, moves, 0);
  }
}

void MoveGen::get_white_pawn_moves() {
  for (const auto one_step :
       BitScan((m_board[Piece::WHITE_PAWN] << 8) & ~masks::RANK_8 &
               ~m_board[Piece::ALL_PCS])) {
    m_movs[m_idx++] = Move::quiet(Piece::WHITE_PAWN, one_step | one_step >> 8,
                                  0, m_board[Piece::INFO]);
  }
  for (const auto one_step_prom :
       BitScan((m_board[Piece::WHITE_PAWN] << 8) & masks::RANK_8 &
               ~m_board[Piece::ALL_PCS])) {
    m_movs[m_idx++] =
        Move::promote(Piece::WHITE_PAWN, one_step_prom >> 8, Piece::WHITE_QUEEN,
                      one_step_prom, 0, m_board[Piece::INFO]);
    m_movs[m_idx++] = Move::promote(Piece::WHITE_PAWN, one_step_prom >> 8,
                                    Piece::WHITE_KNIGHT, one_step_prom, 0,
                                    m_board[Piece::INFO]);
    m_movs[m_idx++] =
        Move::promote(Piece::WHITE_PAWN, one_step_prom >> 8, Piece::WHITE_ROOK,
                      one_step_prom, 0, m_board[Piece::INFO]);
    m_movs[m_idx++] = Move::promote(Piece::WHITE_PAWN, one_step_prom >> 8,
                                    Piece::WHITE_BISHOP, one_step_prom, 0,
                                    m_board[Piece::INFO]);
  }

  const uint64_t two_steps =
      ((m_board[Piece::WHITE_PAWN] & masks::RANK_2) << 16) &
      ~((m_board[Piece::ALL_PCS]) | (m_board[Piece::ALL_PCS] << 8));
  for (const auto two_step : BitScan(two_steps)) {
    m_movs[m_idx++] = Move::quiet(Piece::WHITE_PAWN, two_step | two_step >> 16,
                                  (two_step >> 8), m_board[Piece::INFO]);
  }

  white_pawn_taking_moves(7);
  white_pawn_taking_moves(9);

  const uint64_t en_passent_take_left =
      ((m_board[Piece::WHITE_PAWN] << 9) & m_board[Piece::INFO] &
       ~masks::RANK_1 & ~masks::FILE_H);
  if (en_passent_take_left != 0) {
    m_movs[m_idx++] = Move::capture(
        Piece::WHITE_PAWN, en_passent_take_left | (en_passent_take_left >> 9),
        Piece::BLACK_PAWN, en_passent_take_left >> 8, 0, m_board[Piece::INFO]);
  }

  const uint64_t en_passent_take_right =
      ((m_board[Piece::WHITE_PAWN] << 7) & m_board[Piece::INFO] &
       ~masks::RANK_1 & ~masks::FILE_A);
  if (en_passent_take_right != 0) {
    m_movs[m_idx++] = Move::capture(
        Piece::WHITE_PAWN, en_passent_take_right | (en_passent_take_right >> 7),
        Piece::BLACK_PAWN, en_passent_take_right >> 8, 0, m_board[Piece::INFO]);
  }
}

void MoveGen::get_black_pawn_moves() {

  for (const auto one_step :
       BitScan((m_board[Piece::BLACK_PAWN] >> 8) & ~masks::RANK_1 &
               ~m_board[Piece::ALL_PCS])) {
    m_movs[m_idx++] = Move::quiet(Piece::BLACK_PAWN, one_step | one_step << 8,
                                  0, m_board[Piece::INFO]);
  }

  for (const auto one_step_prom :
       BitScan((m_board[Piece::BLACK_PAWN] >> 8) & masks::RANK_1 &
               ~m_board[Piece::ALL_PCS])) {
    m_movs[m_idx++] =
        Move::promote(Piece::BLACK_PAWN, one_step_prom << 8, Piece::BLACK_QUEEN,
                      one_step_prom, 0, m_board[Piece::INFO]);
    m_movs[m_idx++] = Move::promote(Piece::BLACK_PAWN, one_step_prom << 8,
                                    Piece::BLACK_KNIGHT, one_step_prom, 0,
                                    m_board[Piece::INFO]);
    m_movs[m_idx++] =
        Move::promote(Piece::BLACK_PAWN, one_step_prom << 8, Piece::BLACK_ROOK,
                      one_step_prom, 0, m_board[Piece::INFO]);
    m_movs[m_idx++] = Move::promote(Piece::BLACK_PAWN, one_step_prom << 8,
                                    Piece::BLACK_BISHOP, one_step_prom, 0,
                                    m_board[Piece::INFO]);
  }

  const uint64_t two_steps =
      ((m_board[Piece::BLACK_PAWN] & masks::RANK_7) >> 16) &
      ~((m_board[Piece::ALL_PCS]) | (m_board[Piece::ALL_PCS] >> 8));
  for (const auto two_step : BitScan(two_steps)) {
    m_movs[m_idx++] = Move::quiet(Piece::BLACK_PAWN, two_step | two_step << 16,
                                  (two_step << 8), m_board[Piece::INFO]);
  }

  black_pawn_taking_moves(9);
  black_pawn_taking_moves(7);
  const uint64_t en_passent_take_left =
      ((m_board[Piece::BLACK_PAWN] >> 7) & m_board[Piece::INFO] &
       ~masks::RANK_1 & ~masks::FILE_H);
  if (en_passent_take_left != 0) {
    m_movs[m_idx++] = Move::capture(
        Piece::BLACK_PAWN, en_passent_take_left | (en_passent_take_left << 7),
        Piece::WHITE_PAWN, en_passent_take_left << 8, 0, m_board[Piece::INFO]);
  }

  const uint64_t en_passent_take_right =
      ((m_board[Piece::BLACK_PAWN] >> 9) & m_board[Piece::INFO] &
       ~masks::RANK_1 & ~masks::FILE_A);
  if (en_passent_take_right != 0) {
    m_movs[m_idx++] = Move::capture(
        Piece::BLACK_PAWN, en_passent_take_right | (en_passent_take_right << 9),
        Piece::WHITE_PAWN, en_passent_take_right << 8, 0, m_board[Piece::INFO]);
  }
}

void MoveGen::black_pawn_taking_moves(const int offset) {
  uint64_t file_mask = offset == 7 ? masks::FILE_H : masks::FILE_A;
  for (const auto take_right :
       BitScan((m_board[Piece::BLACK_PAWN] >> offset) &
               m_board[Piece::WHITE_PCS] & ~file_mask)) {
    for (auto const piece : PieceRange::WhiteNoKing()) {
      const uint64_t taken_piece = (take_right & m_board[piece]);
      if (taken_piece == 0) {
        continue;
      }
      const uint64_t promotion_sq = (take_right & masks::RANK_1);
      if (promotion_sq != 0) {
        m_movs[m_idx++] = Move::promote_capture(
            Piece::BLACK_PAWN, take_right << offset, piece, taken_piece,
            Piece::BLACK_QUEEN, promotion_sq, 0, m_board[Piece::INFO]);
        m_movs[m_idx++] = Move::promote_capture(
            Piece::BLACK_PAWN, take_right << offset, piece, taken_piece,
            Piece::BLACK_KNIGHT, promotion_sq, 0, m_board[Piece::INFO]);
        m_movs[m_idx++] = Move::promote_capture(
            Piece::BLACK_PAWN, take_right << offset, piece, taken_piece,
            Piece::BLACK_BISHOP, promotion_sq, 0, m_board[Piece::INFO]);
        m_movs[m_idx++] = Move::promote_capture(
            Piece::BLACK_PAWN, take_right << offset, piece, taken_piece,
            Piece::BLACK_ROOK, promotion_sq, 0, m_board[Piece::INFO]);
      } else {
        m_movs[m_idx++] =
            Move::capture(Piece::BLACK_PAWN, take_right | take_right << offset,
                          piece, taken_piece, 0, m_board[Piece::INFO]);
      }
      break;
    }
  }
}

void MoveGen::white_pawn_taking_moves(const int offset) {
  uint64_t file_mask = offset == 7 ? masks::FILE_A : masks::FILE_H;
  for (const auto take : BitScan((m_board[Piece::WHITE_PAWN] << offset) &
                                 m_board[Piece::BLACK_PCS] & ~file_mask)) {
    for (auto const piece : PieceRange::BlackNoKing()) {
      const uint64_t taken_piece = (take & m_board[piece]);
      if (taken_piece == 0) {
        continue;
      }
      const uint64_t promotion_sq = (take & masks::RANK_8);
      if (promotion_sq != 0) {
        m_movs[m_idx++] = Move::promote_capture(
            Piece::WHITE_PAWN, take >> offset, piece, taken_piece,
            Piece::WHITE_QUEEN, promotion_sq, 0, m_board[Piece::INFO]);
        m_movs[m_idx++] = Move::promote_capture(
            Piece::WHITE_PAWN, take >> offset, piece, taken_piece,
            Piece::WHITE_KNIGHT, promotion_sq, 0, m_board[Piece::INFO]);
        m_movs[m_idx++] = Move::promote_capture(
            Piece::WHITE_PAWN, take >> offset, piece, taken_piece,
            Piece::WHITE_BISHOP, promotion_sq, 0, m_board[Piece::INFO]);
        m_movs[m_idx++] = Move::promote_capture(
            Piece::WHITE_PAWN, take >> offset, piece, taken_piece,
            Piece::WHITE_ROOK, promotion_sq, 0, m_board[Piece::INFO]);
      } else {
        m_movs[m_idx++] =
            Move::capture(Piece::WHITE_PAWN, take | take >> offset, piece,
                          taken_piece, 0, m_board[Piece::INFO]);
      }
      break;
    }
  }
}

void MoveGen::get_white_king_moves() {
  const uint64_t moves =
      data::KING_MOVES[__builtin_ctzll(m_board[Piece::WHITE_KING])] &
      ~m_board[Piece::WHITE_PCS];
  const uint64_t info_xor =
      (castling::WHITE_KINGSIDE_RIGHT | castling::WHITE_QUEENSIDE_RIGHT) &
      m_board[Piece::INFO];

  white_add_to_movs(Piece::WHITE_KING, m_board[Piece::WHITE_KING], moves,
                    info_xor);

  uint64_t attacks = 0;
  if (((m_board[Piece::INFO] & castling::WHITE_KINGSIDE_RIGHT) != 0) &&
      ((castling::WHITE_KINGSIDE_SPACE &
        (m_board[Piece::WHITE_PCS] | m_board[Piece::BLACK_PCS])) == 0) &&
      ((m_board[Piece::WHITE_ROOK] & masks::FILE_H & masks::RANK_1) != 0)) {
    attacks = get_black_attackers(m_board);
    if ((attacks & castling::WHITE_KINGSIDE_ATTACKED) == 0) {
      m_movs[m_idx++] =
          Move::castle_kingside(Piece::WHITE_KING, 0b1010ULL, Piece::WHITE_ROOK,
                                0b0101ULL, info_xor, m_board[Piece::INFO]);
    }
  }
  if (((m_board[Piece::INFO] & castling::WHITE_QUEENSIDE_RIGHT) != 0) &&
      ((castling::WHITE_QUEENSIDE_SPACE &
        (m_board[Piece::WHITE_PCS] | m_board[Piece::BLACK_PCS])) == 0) &&
      ((m_board[Piece::WHITE_ROOK] & masks::FILE_A & masks::RANK_1) != 0)) {
    if (attacks == 0) {
      attacks = get_black_attackers(m_board);
    }

    if ((attacks & castling::WHITE_QUEENSIDE_ATTACKED) == 0) {
      m_movs[m_idx++] = Move::castle_queenside(Piece::WHITE_KING, 0b101000ULL,
                                               Piece::WHITE_ROOK, 0b10010000ULL,
                                               info_xor, m_board[Piece::INFO]);
    }
  }
}

void MoveGen::get_black_king_moves() {
  const uint64_t moves =
      data::KING_MOVES[__builtin_ctzll(m_board[Piece::BLACK_KING])] &
      ~m_board[Piece::BLACK_PCS];

  const uint64_t info_xor =
      (castling::BLACK_KINGSIDE_RIGHT | castling::BLACK_QUEENSIDE_RIGHT) &
      m_board[Piece::INFO];

  black_add_to_movs(Piece::BLACK_KING, m_board[Piece::BLACK_KING], moves,
                    info_xor);

  uint64_t attacks = 0;
  if (((m_board[Piece::INFO] & castling::BLACK_KINGSIDE_RIGHT) != 0) &&
      ((castling::BLACK_KINGSIDE_SPACE &
        (m_board[Piece::WHITE_PCS] | m_board[Piece::BLACK_PCS])) == 0) &&
      ((m_board[Piece::BLACK_ROOK] & masks::FILE_H & masks::RANK_8) != 0)) {
    attacks = get_white_attackers(m_board);
    if ((attacks & castling::BLACK_KINGSIDE_ATTACKED) == 0) {
      m_movs[m_idx++] = Move::castle_kingside(
          Piece::BLACK_KING, 0b1010ULL << 56, Piece::BLACK_ROOK,
          0b0101ULL << 56, info_xor, m_board[Piece::INFO]);
    }
  }

  if (((m_board[Piece::INFO] & castling::BLACK_QUEENSIDE_RIGHT) != 0) &&
      ((castling::BLACK_QUEENSIDE_SPACE &
        (m_board[Piece::WHITE_PCS] | m_board[Piece::BLACK_PCS])) == 0) &&
      ((m_board[Piece::BLACK_ROOK] & masks::FILE_A & masks::RANK_8) != 0)) {
    if (attacks == 0) {
      attacks = get_white_attackers(m_board);
    }

    if ((attacks & castling::BLACK_QUEENSIDE_ATTACKED) == 0) {
      m_movs[m_idx++] = Move::castle_queenside(
          Piece::BLACK_KING, 0b101000ULL << 56, Piece::BLACK_ROOK,
          0b10010000ULL << 56, info_xor, m_board[Piece::INFO]);
    }
  }
}

void MoveGen::get_white_queen_moves() {
  for (const auto queen : BitScan(m_board[Piece::WHITE_QUEEN])) {
    const uint64_t moves =
        get_white_bishop_attacks(queen) | get_white_rook_attacks(queen);

    white_add_to_movs(Piece::WHITE_QUEEN, queen, moves, 0);
  }
}

void MoveGen::get_black_queen_moves() {
  for (const auto queen : BitScan(m_board[Piece::BLACK_QUEEN])) {
    const uint64_t moves =
        get_black_bishop_attacks(queen) | get_black_rook_attacks(queen);

    black_add_to_movs(Piece::BLACK_QUEEN, queen, moves, 0);
  }
}

void MoveGen::white_add_to_movs(const Piece moving_pc,
                                const uint64_t moving_pc_spot,
                                const uint64_t moves, const uint64_t info) {
  const uint64_t board_info = m_board[Piece::INFO];

  for (const auto mov : BitScan(moves & ~m_board[Piece::BLACK_PCS])) {
    m_movs[m_idx++] =
        Move::quiet(moving_pc, mov | moving_pc_spot, info, board_info);
  }

  for (const auto taking_spot : BitScan(moves & m_board[Piece::BLACK_PCS])) {
    for (const auto taken_pc : PieceRange::BlackNoKing()) {
      const uint64_t taken_spot = (taking_spot & m_board[taken_pc]);
      if (taken_spot != 0) {
        m_movs[m_idx++] = Move::capture(moving_pc, taken_spot | moving_pc_spot,
                                        taken_pc, taken_spot, info, board_info);
        break;
      }
    }
  }
}

void MoveGen::black_add_to_movs(const Piece moving_pc,
                                const uint64_t moving_pc_spot,
                                const uint64_t moves, const uint64_t info) {
  const uint64_t board_info = m_board[Piece::INFO];
  for (const auto mov : BitScan(moves & ~m_board[Piece::WHITE_PCS])) {
    m_movs[m_idx++] =
        Move::quiet(moving_pc, mov | moving_pc_spot, info, board_info);
  }

  for (const auto taking_spot : BitScan(moves & m_board[Piece::WHITE_PCS])) {
    for (const auto taken_pc : PieceRange::WhiteNoKing()) {
      const uint64_t taken_spot = (taking_spot & m_board[taken_pc]);
      if (taken_spot != 0) {
        m_movs[m_idx++] = Move::capture(moving_pc, taken_spot | moving_pc_spot,
                                        taken_pc, taken_spot, info, board_info);
        break;
      }
    }
  }
}

auto MoveGen::get_white_attackers(const BitBoard &m_board) -> uint64_t {
  uint64_t attacks = 0;
  attacks |= ((m_board[Piece::WHITE_PAWN] & ~masks::FILE_H) << 7) |
             ((m_board[Piece::WHITE_PAWN] & ~masks::FILE_A) << 9);

  attacks |= data::KING_MOVES[__builtin_ctzll(m_board[Piece::WHITE_KING])];

  for (const auto piece : BitScan(m_board[Piece::WHITE_KNIGHT])) {
    attacks |= data::KNIGHT_MOVES[__builtin_ctzll(piece)];
  }

  for (const auto piece :
       BitScan((m_board[Piece::WHITE_BISHOP] | m_board[Piece::WHITE_QUEEN]))) {
    attacks |= get_white_bishop_attacks(piece);
  }

  for (const auto piece :
       BitScan((m_board[Piece::WHITE_ROOK] | m_board[Piece::WHITE_QUEEN]))) {
    attacks |= get_white_rook_attacks(piece);
  }

  return attacks;
}

auto MoveGen::get_black_attackers(const BitBoard &m_board) -> uint64_t {

  uint64_t attacks = 0;

  attacks |= ((m_board[Piece::BLACK_PAWN] & ~masks::FILE_H) >> 9) |
             ((m_board[Piece::BLACK_PAWN] & ~masks::FILE_A) >> 7);

  attacks |= data::KING_MOVES[__builtin_ctzll(m_board[Piece::BLACK_KING])];

  for (const auto piece : BitScan(m_board[Piece::BLACK_KNIGHT])) {
    attacks |= data::KNIGHT_MOVES[__builtin_ctzll(piece)];
  }

  for (const auto piece :
       BitScan(m_board[Piece::BLACK_BISHOP] | m_board[Piece::BLACK_QUEEN])) {
    attacks |= get_black_bishop_attacks(piece);
  }

  for (const auto piece :
       BitScan(m_board[Piece::BLACK_ROOK] | m_board[Piece::BLACK_QUEEN])) {
    attacks |= get_black_rook_attacks(piece);
  }

  return attacks;
}

// NOLINTBEGIN
auto MoveGen::is_white_king_in_check() -> bool {
  if (((m_board[Piece::BLACK_ROOK] | m_board[Piece::BLACK_QUEEN]) &
       get_white_rook_attacks(m_board[Piece::WHITE_KING])) != 0) {
    return true;
  }
  if (((m_board[Piece::BLACK_BISHOP] | m_board[Piece::BLACK_QUEEN]) &
       get_white_bishop_attacks(m_board[Piece::WHITE_KING])) != 0) {
    return true;
  }
  if ((m_board[Piece::BLACK_KNIGHT] &
       data::KNIGHT_MOVES[__builtin_ctzll(m_board[Piece::WHITE_KING])]) != 0) {
    return true;
  }
  if ((m_board[Piece::BLACK_KING] &
       data::KING_MOVES[__builtin_ctzll(m_board[Piece::WHITE_KING])]) != 0) {
    return true;
  }
  if (((((m_board[Piece::WHITE_KING] << 9) & ~masks::FILE_H) |
        ((m_board[Piece::WHITE_KING] << 7) & ~masks::FILE_A)) &
       m_board[Piece::BLACK_PAWN]) != 0) {
    return true;
  }
  return false;
}

auto MoveGen::is_black_king_in_check() -> bool {
  if (((m_board[Piece::WHITE_ROOK] | m_board[Piece::WHITE_QUEEN]) &
       get_black_rook_attacks(m_board[Piece::BLACK_KING])) != 0) {
    return true;
  }
  if (((m_board[Piece::WHITE_BISHOP] | m_board[Piece::WHITE_QUEEN]) &
       get_black_bishop_attacks(m_board[Piece::BLACK_KING])) != 0) {
    return true;
  }
  if ((m_board[Piece::WHITE_KNIGHT] &
       data::KNIGHT_MOVES[__builtin_ctzll(m_board[Piece::BLACK_KING])]) != 0) {
    return true;
  }
  if (((m_board[Piece::WHITE_KING] &
        data::KING_MOVES[__builtin_ctzll(m_board[Piece::BLACK_KING])])) != 0) {
    return true;
  }
  if (((((m_board[Piece::BLACK_KING] >> 9) & ~masks::FILE_A) |
        ((m_board[Piece::BLACK_KING] >> 7) & ~masks::FILE_H)) &
       m_board[Piece::WHITE_PAWN]) != 0) {
    return true;
  }
  return false;
}
// NOLINTEND

auto MoveGen::compare_moves(const Move &mov_a, const Move &mov_b) -> bool {
  // First compare move types
  if (mov_a.type != mov_b.type) {
    return mov_a.type > mov_b.type; // Higher type comes first
  }

  // If move types are the same, compare based on move type
  switch (mov_a.type) {
  case movType::QUIET:
    return mov_a.pc1 > mov_b.pc1; // Higher pc2 comes first

  case movType::CAPTURE:
    // Primary: compare captured pieces (pc2)
    if (mov_b.pc2 != mov_a.pc2) {
      return mov_a.pc2 > mov_b.pc2; // Higher pc2 comes first
    }
    // Secondary: compare capturing pieces (pc1)
    return mov_b.pc1 > mov_a.pc1; // Lower pc1 comes first

  case movType::PROMOTE:
    return mov_a.pc2 > mov_b.pc2; // Higher promotion piece comes first

  case movType::CAPTURE_PROMOTE:
    // Primary: compare promotion piece (pc3)
    if (mov_b.pc3 != mov_a.pc3) {
      return mov_a.pc3 > mov_b.pc3; // Higher pc3 comes first
    }
    // Secondary: compare captured pieces (pc2)
    return mov_a.pc2 > mov_b.pc2; // Higher pc2 comes first

  default:
    return false; // Equal (maintains stable sort)
  }
}

void MoveGen::gen() {
  if (m_board.whites_turn()) {
    get_white_queen_moves();
    get_white_rook_moves();
    get_white_bishop_moves();
    get_white_knight_moves();
    get_white_pawn_moves();
    get_white_king_moves();
  } else {
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