#include "pv.h"

#include <algorithm>
#include <cassert>
#include <ranges>
#include <span>

using namespace std;

void PVTable::update(int ply, const Move& mov)
{
    assert(ply + 1 < static_cast<int>(max_ply));
    assert(m_lengths[ply + 1] < max_ply - 1);

    m_table.at(ply).at(0) = mov;
    for (size_t idx = 0; idx < m_lengths.at(ply + 1); ++idx)
    {
        m_table.at(ply).at(idx + 1) = m_table.at(ply + 1).at(idx);
    }

    m_lengths.at(ply) = m_lengths.at(ply + 1) + 1;
}

auto PVTable::get_pv_at_ply(int ply) const -> std::span<const Move>
{
    return span{m_table.at(ply)}.subspan(0, m_lengths.at(ply));
}

auto PVTable::best_move() const -> const Move& { return m_table.at(0).at(0); }

void PVTable::clear(int ply) { m_lengths.at(ply) = 0; }
