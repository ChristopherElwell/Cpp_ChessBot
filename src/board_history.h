#ifndef BOARD_HISTORY_H
#define BOARD_HISTORY_H

#include <array>

#include "zobrist.h"
struct history_entry
{
    ZobristHash hash = {};
    bool b_irriversible = false;
};

class BoardHistory
{
private:
    static constexpr size_t max_length = 50;
    std::array<history_entry, max_length> m_history = {};
    int m_end = 0;

public:
    void push_back(ZobristHash hash);
    auto pop_back() -> ZobristHash;
    void push_irreversible(ZobristHash hash);
    auto is_threefold(ZobristHash hash) -> bool;
};
#endif
