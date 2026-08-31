#include "board_history.h"

#include <cassert>

#include "zobrist.h"

void BoardHistory::push_back(ZobristHash hash)
{
    m_history.at(m_end++) = hash;
    assert(m_end < static_cast<int>(max_length));
}

void BoardHistory::push_irreversible(ZobristHash hash)
{
    m_end = 1;
    m_history.at(0) = hash;
}

auto BoardHistory::pop_back() -> ZobristHash
{
    m_end--;
    assert(m_end > 0);
    return m_history.at(m_end);
}

auto BoardHistory::is_threefold(ZobristHash hash) -> bool
{
    int occurrences = 0;

    for (int index = m_end - 1; index >= 0; index -= 2)
    {
        if (m_history[index].get() != hash.get())
        {
            continue;
        }
        if (++occurrences == 2)
        {
            return true;
        }
    }

    return false;
}
