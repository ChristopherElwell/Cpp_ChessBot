#pragma once
#include "bitboard.h"
#include "move_gen.h"
#include "search.h"
#include <iostream>
#include <cstdint>

namespace helpers {
    struct BitScan {
        public:
            struct Iterator {
                uint64_t mask;
                uint64_t bit;
                uint64_t operator*() const { return bit; }
                Iterator& operator++() { mask &= (mask - 1); bit = mask & -mask; return *this; }
                bool operator!=(const Iterator& other) const {return mask != other.mask; }
            };

            Iterator begin() const { return Iterator{start, start & -start}; }
            Iterator end() const { return Iterator{0,0}; }

            BitScan(uint64_t mask) : start(mask) {};

        private:
            uint64_t start;
    };

    void free_search_result(SearchRes* sr);
    bool compare_moves(const Move& a, const Move& b) ;
    void print_bitboard(const uint64_t& b);
    uint64_t sq_from_name(char file, char rank);
    void print_move_verbose(const Move& m);
    std::string move_to_uci(const Move& mov, const BitBoard& board);
    void print_principal_variation(const SearchRes* sr, BitBoard bb);
}

std::ostream& operator<<(std::ostream& os, Piece p);
std::ostream& operator<<(std::ostream& os, movType m);
std::ostream& operator<<(std::ostream& os, const Move& m);
std::ostream& operator<<(std::ostream& os, const BitBoard& bb);