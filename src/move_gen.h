#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <ranges>

#include "bitboard.h"
#include "data.h"
#include "move.h"

static constexpr int moves_array_length = 230;

template <side_t Side>
struct side_traits
{
    static constexpr uint64_t rank_1 = (Side == side_t::white) ? masks::rank_1 : masks::rank_8;
    static constexpr uint64_t rank_2 = (Side == side_t::white) ? masks::rank_2 : masks::rank_7;
    static constexpr uint64_t rank_3 = (Side == side_t::white) ? masks::rank_3 : masks::rank_6;
    static constexpr uint64_t rank_4 = (Side == side_t::white) ? masks::rank_4 : masks::rank_5;
    static constexpr uint64_t rank_5 = (Side == side_t::white) ? masks::rank_5 : masks::rank_4;
    static constexpr uint64_t rank_6 = (Side == side_t::white) ? masks::rank_6 : masks::rank_3;
    static constexpr uint64_t rank_7 = (Side == side_t::white) ? masks::rank_7 : masks::rank_2;
    static constexpr uint64_t rank_8 = (Side == side_t::white) ? masks::rank_8 : masks::rank_1;

    static constexpr int pawn_push_dir = (Side == side_t::white) ? 8 : -8;

    static constexpr int pawn_take_w = (Side == side_t::white) ? 9 : -7;
    static constexpr uint64_t pawn_take_w_wrap_mask = masks::file_h;

    static constexpr int pawn_take_e = (Side == side_t::white) ? 7 : -9;
    static constexpr uint64_t pawn_take_e_wrap_mask = masks::file_a;

    static constexpr uint64_t kingside_rook =
        (Side == side_t::white) ? masks::file_h & masks::rank_1 : masks::file_h & masks::rank_8;
    static constexpr uint64_t queenside_rook =
        (Side == side_t::white) ? masks::file_a & masks::rank_1 : masks::file_a & masks::rank_8;
};

struct scored_move
{
    int score;
    Move move;
};

class move_iterator
{
public:
    explicit move_iterator(const scored_move *ptr) : m_ptr(ptr) {}

    auto operator*() const -> const Move & { return m_ptr->move; }
    auto operator++() -> move_iterator &
    {
        ++m_ptr;
        return *this;
    }

    friend auto operator!=(move_iterator lhs, move_iterator rhs) -> bool
    {
        return lhs.m_ptr != rhs.m_ptr;
    }

private:
    const scored_move *m_ptr;
};

class MoveGen
{
private:
    std::array<scored_move, moves_array_length> m_movs;
    size_t m_idx = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    const BitBoard &m_board;
    ptrdiff_t m_end_idx = 0;

    template <side_t Side>
    void add_to_movs(piece_t moving_pc, uint64_t moving_pc_spot, uint64_t moves);

    template <side_t Side>
    [[nodiscard]] auto get_rook_attacks(uint64_t rook) const -> uint64_t;
    template <side_t Side>
    [[nodiscard]] auto get_bishop_attacks(uint64_t bishop) const -> uint64_t;

    template <side_t Side>
    void get_pawn_moves();
    template <side_t Side>
    void pawn_taking_moves(int offset);
    template <side_t Side>
    void get_knight_moves();
    template <side_t Side>
    void get_bishop_moves();
    template <side_t Side>
    void get_rook_moves();
    template <side_t Side>
    void get_queen_moves();
    template <side_t Side>
    void get_king_moves();

    static auto score_move(move_type_t type, piece_t moving_pc,
                           piece_t capturing_pc = piece_t::none) -> int;
    static auto to_move(const scored_move &move) -> const Move & { return move.move; }

public:
    [[nodiscard]] auto length() const -> int { return static_cast<int>(m_end_idx); }

    auto at(size_t idx) -> Move &;
    [[nodiscard]] auto at(size_t idx) const -> const Move &;

    template <side_t Side>
    [[nodiscard]] auto is_king_in_check() const -> bool;

    template <side_t Side>
    auto get_attackers() -> uint64_t;

    template <side_t Side>
    void gen();

    MoveGen(const BitBoard &board);

    auto operator[](int idx) -> Move { return m_movs[idx].move; }

    ~MoveGen() = default;
    MoveGen(const MoveGen &) = delete;
    auto operator=(const MoveGen &) -> MoveGen & = delete;
    MoveGen(MoveGen &&) = delete;
    auto operator=(MoveGen &&other) -> MoveGen & = delete;

    [[nodiscard]] auto begin() const -> move_iterator { return move_iterator(m_movs.data()); }
    [[nodiscard]] auto end() const -> move_iterator
    {
        return move_iterator(m_movs.data() + m_end_idx);
    }
};
