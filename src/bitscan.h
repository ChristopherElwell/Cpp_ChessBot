#pragma once
#include <cstdint>

struct bit_scan
{
public:
    struct iterator
    {
    private:
        uint64_t m_mask;
        uint64_t m_bit;

    public:
        iterator(uint64_t mask, uint64_t bit) : m_mask(mask), m_bit(bit) {}
        auto operator*() const -> uint64_t { return m_bit; }
        auto operator++() -> iterator &
        {
            m_mask &= (m_mask - 1);
            m_bit = m_mask & -m_mask;
            return *this;
        }
        auto operator!=(const iterator &other) const -> bool { return m_mask != other.m_mask; }
    };

    [[nodiscard]] auto begin() const -> iterator { return iterator{m_start, m_start & -m_start}; }
    [[nodiscard]] auto static end() -> iterator { return iterator{0, 0}; }

    bit_scan(uint64_t mask) : m_start(mask) {};

private:
    uint64_t m_start;
};
