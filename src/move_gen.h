#pragma once
#include "bitboard.h"
#include "data.h"
#include "move.h"
#include <array>
#include <cstddef>
#include <cstdint>

class MoveGen {
private:
  std::array<Move, MOVES_ARRAY_LENGTH> m_movs;
  size_t m_idx = 0;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
  const BitBoard &m_board;
  ptrdiff_t m_end_idx = 0;

  void black_add_to_movs(Piece moving_pc, uint64_t moving_pc_spot,
                         uint64_t moves, uint64_t info = 0);
  void white_add_to_movs(Piece moving_pc, uint64_t moving_pc_spot,
                         uint64_t moves, uint64_t info = 0);

  [[nodiscard]] auto get_white_rook_attacks(uint64_t rook) const -> uint64_t;
  [[nodiscard]] auto get_black_rook_attacks(uint64_t rook) const -> uint64_t;
  [[nodiscard]] auto get_white_bishop_attacks(uint64_t bishop) const
      -> uint64_t;
  [[nodiscard]] auto get_black_bishop_attacks(uint64_t bishop) const
      -> uint64_t;

  void get_white_knight_moves();
  void get_black_knight_moves();

  void get_white_bishop_moves();
  void get_black_bishop_moves();

  void get_white_rook_moves();
  void get_black_rook_moves();

  void get_white_pawn_moves();
  void get_black_pawn_moves();

  void white_pawn_taking_moves(int offset);
  void black_pawn_taking_moves(int offset);

  void get_white_queen_moves();
  void get_black_queen_moves();

  void get_white_king_moves();
  void get_black_king_moves();

  void get_white_moves();
  void get_black_moves();

  static auto compare_moves(const Move &mov_a, const Move &mov_b) -> bool;

public:
  [[nodiscard]]
  auto length() const -> int {
    return static_cast<int>(m_end_idx);
  }

  auto is_white_king_in_check() -> bool;
  auto is_black_king_in_check() -> bool;

  auto get_white_attackers(const BitBoard &board) -> uint64_t;
  auto get_black_attackers(const BitBoard &board) -> uint64_t;
  void gen();
  MoveGen(const BitBoard &board);

  auto operator[](int idx) -> Move { return m_movs[idx]; }

  ~MoveGen() = default;
  MoveGen(const MoveGen &) = delete;
  auto operator=(const MoveGen &) -> MoveGen & = delete;
  MoveGen(MoveGen &&) = delete;
  auto operator=(MoveGen &&other) -> MoveGen & = delete;

  [[nodiscard]] auto begin() const -> std::array<Move, 230>::const_iterator {
    return m_movs.begin();
  }
  [[nodiscard]] auto end() const -> std::array<Move, 230>::const_iterator {
    return m_movs.begin() + m_end_idx;
  }
};
