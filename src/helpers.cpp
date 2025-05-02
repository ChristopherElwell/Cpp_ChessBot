#include "helpers.h"
#include "move_gen.h"
#include "bitboard.h"
#include "move.h"
#include "data.h"
#include <iostream>
#include <string>

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


bool compare_moves(const Move& a, const Move& b) {
    // First compare move types
    if (a.type != b.type) {
        return a.type > b.type;  // Higher type comes first
    }
    
    // If move types are the same, compare based on move type
    switch (a.type) {
        case movType::QUIET:
            return a.pc1 > b.pc1;  // Higher pc2 comes first
            
        case movType::CAPTURE:
            // Primary: compare captured pieces (pc2)
            if (b.pc2 != a.pc2) {
                return a.pc2 > b.pc2;  // Higher pc2 comes first
            }
            // Secondary: compare capturing pieces (pc1)
            return b.pc1 > a.pc1;  // Lower pc1 comes first
            
        case movType::PROMOTE:
            return a.pc2 > b.pc2;  // Higher promotion piece comes first
            
        case movType::CAPTURE_PROMOTE:
            // Primary: compare promotion piece (pc3)
            if (b.pc3 != a.pc3) {
                return a.pc3 > b.pc3;  // Higher pc3 comes first
            }
            // Secondary: compare captured pieces (pc2)
            return a.pc2 > b.pc2;  // Higher pc2 comes first
            
        default:
            return false;  // Equal (maintains stable sort)
    }
}

uint64_t sq_from_name(char file, char rank){
    return masks::RANKS[rank - '1'] & masks::FILES['h' - file];
}

void print_bitboard(const uint64_t& b){
    for (int i = 0; i < 64; i++){
        if(i%8 == 0){
            std::cout << 8 - i/8 << " ";
        }
        if (b & (1ULL << (63 - i))){
            std::cout << WHITE_SQ_CHAR;
        } else {
            std::cout << BLACK_SQ_CHAR;
        }
        if ((i+1)%8 == 0){
            std::cout << "\n";
        }
    }
    std::cout << "  a b c d e f g h\n\n";
}

std::ostream& operator<<(std::ostream& os, Piece p) {
    return os  << pc_chars[static_cast<int>(p)] << " " ;
}

std::ostream& operator<<(std::ostream& os, movType m) {
    switch (m) {
        case movType::QUIET: return os << "Quiet";
        case movType::CAPTURE: return os << "Capture";
        case movType::PROMOTE: return os << "Promote";
        case movType::CAPTURE_PROMOTE: return os << "Capture & Promote";
        case movType::CASTLE_KINGSIDE: return os << "Castle Kingside";
        case movType::CASTLE_QUEENSIDE: return os << "Castle Queenside";
        case movType::BOOK_END: return os << "Bookend";
    }
    return os << "Unknown";
}

std::ostream& operator<<(std::ostream& os, const Move& m){
    switch(m.type){
        case movType::QUIET:
            return os << "Quiet " << m.pc1 << "\n";
        case movType::CAPTURE:
            return os << m.pc1 << " takes " << m.pc2 << "\n";
        case movType::PROMOTE:
            return os << m.pc1 << " promotes to " << m.pc2 << "\n";
        case movType::CAPTURE_PROMOTE:
            return os << m.pc1 << " takes " << m.pc2 << " and promotes to " << m.pc3 << "\n";
        case movType::CASTLE_KINGSIDE:
            return os << "O-O\n";
        case movType::CASTLE_QUEENSIDE:
            return os << "O-O-O\n";
        case movType::BOOK_END:
            return os << "Bookend\n";
    }
    return os << "unknown move";
}

std::ostream& operator<<(std::ostream& os, const BitBoard& bb){
    std::string reset = "\033[0m";
    std::string light = "\033[48;5;187m";
    std::string dark  = "\033[48;5;66m";
    os << "8 ";
    for (int i = 0; i < 64; i++){
        std::cout << ((i%8 + i/8)%2 == 0 ? light : dark);
        int piece_found = 0;
        for (Piece pc : PieceRange::all()){
            if (bb[pc] & (1ULL << (63 - i))){
                os << pc;
                piece_found = 1;
                break;
            } 
        }
        if (!piece_found){
            os << "  ";
        }
        if ((i+1)%8 == 0 && i != 63){
            os << reset << "\n" << 8 - (i + 1)/8 << " ";
        } 
    }
    return os << reset << "\n  a b c d e f g h\n\n";
}

void print_move_short(const Move& m){
    std::cout << "Move Type: " << m.type << "\n";
    std::cout << "Primary Piece: " << m.pc1;

    print_bitboard(m.mov1);
    
    switch (m.type){
        case movType::QUIET:
        case movType::CASTLE_KINGSIDE:
        case movType::CASTLE_QUEENSIDE:
        case movType::BOOK_END:
        case movType::CAPTURE:
            std::cout << "Captured Piece: " << m.pc2;
            print_bitboard(m.mov2);
        case movType::PROMOTE:
            std::cout << "Promotee Piece: " << m.pc2;
            print_bitboard(m.mov2);
        case movType::CAPTURE_PROMOTE:
            std::cout << "Captured Piece: " << m.pc2;
            print_bitboard(m.mov2);
            std::cout << "Promotee Piece: " << m.pc3;
            print_bitboard(m.mov3);
    }
}

std::string move_to_uci(const Move& mov, const BitBoard& board){
    if (mov.type == movType::BOOK_END){
        return "BOOKEND";
    }
    std::string out;
    uint64_t starting_sq = mov.mov1 & board[mov.pc1];
    uint64_t ending_sq = mov.mov1 & ~board[mov.pc1];
    if (starting_sq == 0 || ending_sq == 0) {
        return "INVALID";
    }
    out += data::SQUARES[__builtin_ctzll(starting_sq)];
    out += data::SQUARES[__builtin_ctzll(ending_sq)];
    switch (mov.type){
        case movType::QUIET:
        case movType::CAPTURE:
        case movType::CASTLE_KINGSIDE:
        case movType::CASTLE_QUEENSIDE:
            return out;
        case movType::PROMOTE:
            out += data::PIECE_CODES[static_cast<int>(mov.pc2)];
            return out;
        case movType::CAPTURE_PROMOTE:
            out += data::PIECE_CODES[static_cast<int>(mov.pc3)];
            return out;
        default:
            return "UNKNOWN";
    }

    return out;
}