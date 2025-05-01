#include "helpers.h"
#include "move_gen.h"
#include "bitboard.h"
#include "move.h"
#include "data.h"
#include <iostream>
#include <string>

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
    return os << pc_chars[static_cast<int>(p)];
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

    os << "Move Type: " << m.type << "\n";
    os << "Primary Piece: " << m.pc1;

    print_bitboard(m.mov1);
    
    switch (m.type){
        case movType::QUIET:
        case movType::CASTLE_KINGSIDE:
        case movType::CASTLE_QUEENSIDE:
        case movType::BOOK_END:
        return os;
        case movType::CAPTURE:
            os << "Captured Piece: " << m.pc2;
            print_bitboard(m.mov2);
            return os;
        case movType::PROMOTE:
            os << "Promotee Piece: " << m.pc2;
            print_bitboard(m.mov2);
            return os;
        case movType::CAPTURE_PROMOTE:
            os << "Captured Piece: " << m.pc2;
            print_bitboard(m.mov2);
            os << "Promotee Piece: " << m.pc3;
            print_bitboard(m.mov3);
            return os;
    }

    return os << "unknown";
}

std::ostream& operator<<(std::ostream& os, const BitBoard& bb){
    for (int i = 0; i < 64; i++){
        if(i%8 == 0){
            os << 8 - i/8;
        } else if ((i+1)%8 == 0){
            os << "\n";
        } else {
            for (Piece pc : PieceRange::all()){
                if (bb[pc] & (1ULL << (63 - i))){
                    os << pc;
                    break;
                } else {
                    os << " ";
                }
            }
        }
    }
    return os << "  a b c d e f g h\n\n";
}

void print_move_short(const Move& m){
    switch(m.type){
        case movType::QUIET:
            std::cout << "Quiet " << m.pc1 << "\n";
            return;
        case movType::CAPTURE:
            std::cout << m.pc1 << " takes " << m.pc2 << "\n";
            return;
        case movType::PROMOTE:
            std::cout << m.pc1 << " promotes to " << m.pc2 << "\n";
            return;
        case movType::CAPTURE_PROMOTE:
            std::cout << m.pc1 << " takes " << m.pc2 << " and promotes to " << m.pc3 << "\n";
            return;
        case movType::CASTLE_KINGSIDE:
            std::cout << "O-O\n";
            return;
        case movType::CASTLE_QUEENSIDE:
            std::cout << "O-O-O\n";
            return;
        case movType::BOOK_END:
            std::cout << "Bookend\n";
            return;
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