#ifndef PV_H
#define PV_H

#include <array>
#include <span>

#include "move.h"

class PVTable
{
private:
    static constexpr size_t max_ply = 32;
    std::array<std::array<Move, max_ply>, max_ply> m_table = {};
    std::array<size_t, max_ply> m_lengths = {};

public:
    void update(int ply, const Move& mov);
    [[nodiscard]] auto get_pv_at_ply(int ply) const -> std::span<const Move>;
    [[nodiscard]] auto best_move() const -> const Move&;
    void clear(int ply);
};

#endif
