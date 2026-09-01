#include "board_history.h"

#include <cassert>

#include "zobrist.h"

void BoardHistory::push_back(ZobristHash hash)
{
    m_history.at(m_end++) = history_entry{.hash = hash, .b_irriversible = false};
    assert(m_end < static_cast<int>(max_length));
}

void BoardHistory::push_irreversible(ZobristHash hash)
{
    m_history.at(m_end++) = history_entry{.hash = hash, .b_irriversible = true};
    assert(m_end < static_cast<int>(max_length));
}

auto BoardHistory::pop_back() -> ZobristHash
{
    m_end--;
    assert(m_end > 0);
    return m_history.at(m_end).hash;
}

auto BoardHistory::is_threefold(ZobristHash hash) -> bool
{
    int occurrences = 0;
    bool b_irreversible_found = false;

    for (int index = m_end - 1; index >= 0; index--)
    {
        if (m_history[index].b_irriversible)
        {
            b_irreversible_found = true;
        }
        if (m_history[index].hash.get() != hash.get())
        {
            continue;
        }
        if (++occurrences == 2)
        {
            return true;
        }
        if (b_irreversible_found)
        {
            return false;
        }
    }

    return false;
}
