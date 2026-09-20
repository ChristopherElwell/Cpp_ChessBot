#include "move.h"

#include <bit>
#include <cassert>
#include <cstdint>

#include "bitboard.h"
#include "bitboard_constants.h"

using namespace std;

Move::Move(int sq_from, int sq_to, move_type_t type)
    : m_mask((sq_from << from_shift) | (sq_to << to_shift) | static_cast<uint16_t>(type))
{
}

Move::Move(uint64_t sq_from, uint64_t sq_to, move_type_t type)
    : m_mask((countr_zero(sq_from) << from_shift) | (countr_zero(sq_to) << to_shift) |
             static_cast<uint16_t>(type))
{
    assert(std::popcount(sq_from) == 1 && std::popcount(sq_to) == 1);
}

auto Move::type() const -> move_type_t { return static_cast<move_type_t>(m_mask & type_mask); }

auto Move::from() const -> int { return (m_mask >> from_shift) & from_mask; }

auto Move::to() const -> int { return (m_mask >> to_shift) & to_mask; }

auto Move::operator==(Move other) const -> bool { return m_mask == other.m_mask; }
