#pragma once
#include <cstdint>

struct BitScan
{
   public:
    struct Iterator
    {
       private:
        uint64_t mask;
        uint64_t bit;

       public:
        Iterator(uint64_t mask, uint64_t bit) : mask(mask), bit(bit) {}
        auto operator*() const -> uint64_t { return bit; }
        auto operator++() -> Iterator &
        {
            mask &= (mask - 1);
            bit = mask & -mask;
            return *this;
        }
        auto operator!=(const Iterator &other) const -> bool { return mask != other.mask; }
    };

    [[nodiscard]] auto begin() const -> Iterator { return Iterator{start, start & -start}; }
    [[nodiscard]] auto static end() -> Iterator { return Iterator{0, 0}; }

    BitScan(uint64_t mask) : start(mask) {};

   private:
    uint64_t start;
};