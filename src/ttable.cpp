#include "ttable.h"

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "eval.h"
#include "search.h"

using namespace std;

auto TTable::probe(uint64_t key, const search_args& args) -> tt_result
{
    tt_node& node = m_table[key & m_mask];
    if (node.key != key)
    {
        return {.node = nullptr, .result = tt_probe_result::miss};
    }

    if (node.depth < args.depth)
    {
        return {.node = &node, .result = tt_probe_result::move};
    }

    if (node.flag == tt_node_flag::exact)
    {
        return {.node = &node, .result = tt_probe_result::eval};
    }

    if (node.flag == tt_node_flag::lower_bound && node.eval >= args.beta)
    {
        return {.node = &node, .result = tt_probe_result::eval};
    }

    if (node.flag == tt_node_flag::upper_bound && node.eval <= args.alpha)
    {
        return {.node = &node, .result = tt_probe_result::eval};
    }

    return {.node = &node, .result = tt_probe_result::move};
}

void TTable::store(tt_node node, int16_t alpha, int16_t beta, int8_t ply)
{
    tt_node& old_node = m_table[node.key & m_mask];
    if (old_node.depth > node.depth)
    {
        return;
    }

    if (is_mate_eval(node.eval))
    {
        node.eval = static_cast<int16_t>(
            node.eval + ((node.eval > 0) ? static_cast<int16_t>(ply) : static_cast<int16_t>(-ply)));
    }
    if (node.eval >= beta)
    {
        node.flag = tt_node_flag::lower_bound;
    }
    else if (node.eval <= alpha)
    {
        node.flag = tt_node_flag::upper_bound;
    }
    else
    {
        node.flag = tt_node_flag::exact;
    }
    old_node = node;
}

void TTable::set_size(size_t size)
{
    assert(popcount(size) == 1);
    m_table = vector<tt_node>(size);
    m_mask = size - 1;
    m_size = size;
}

void TTable::clear() { std::ranges::fill(m_table, tt_node{}); }

auto TTable::size() const -> size_t { return m_size; }
