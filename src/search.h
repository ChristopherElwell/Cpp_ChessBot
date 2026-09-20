#ifndef SEARCH_H
#define SEARCH_H

#include <cstdint>

struct search_args
{
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    int8_t depth;
    int8_t ply;
    int16_t alpha;
    int16_t beta;
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    [[nodiscard]] auto next() const -> search_args
    {
        return search_args{.depth = static_cast<int8_t>(depth - 1),
                           .ply = static_cast<int8_t>(ply + 1),
                           .alpha = static_cast<int16_t>(-beta),
                           .beta = static_cast<int16_t>(-alpha)};
    }
};

#endif
