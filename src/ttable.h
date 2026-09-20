#ifndef TTABLE_H
#define TTABLE_H

#include <cstdint>
#include <optional>
#include <vector>

#include "move.h"
#include "search.h"

enum class tt_probe_result : uint8_t
{
    miss,
    move,
    eval
};

enum class tt_node_flag : uint8_t
{
    exact,
    lower_bound,
    upper_bound,
};

struct tt_node
{
    uint64_t key;
    Move best_move;
    int16_t eval;
    int8_t depth;
    tt_node_flag flag;
};

struct tt_result
{
    tt_node* node;
    tt_probe_result result;
};

class TTable
{
private:
    std::vector<tt_node> m_table;
    uint64_t m_mask = 0;
    size_t m_size = 0;
    static constexpr size_t default_size_pow2 = 22;
    static constexpr size_t default_size = 1 << default_size_pow2;

public:
    void set_size(size_t size = default_size);
    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto probe(uint64_t key, const search_args& args) -> tt_result;
    void store(tt_node node, int16_t alpha, int16_t beta, int8_t ply);
    void clear();
};

#endif
