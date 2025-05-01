#include "bitboard.h"
#include "data.h"
#include "move.h"
#include "move_gen.h"
#include "helpers.h"
#include <cstdint>
#include <array>
#include <unordered_map>

uint64_t& BitBoard::operator[](Piece p){
    return board[static_cast<int>(p)];
}

const uint64_t& BitBoard::operator[](Piece p) const {
    return board[static_cast<int>(p)];
}

BitBoard::BitBoard(const std::string& FEN){
    int sq = 0;
    std::unordered_map<char,Piece> Piece_code_map = {
        {'P',Piece::WHITE_PAWN},
        {'N',Piece::WHITE_KNIGHT},
        {'B',Piece::WHITE_BISHOP},
        {'R',Piece::WHITE_ROOK},
        {'Q',Piece::WHITE_QUEEN},
        {'K',Piece::WHITE_KING},
        {'p',Piece::BLACK_PAWN},
        {'n',Piece::BLACK_KNIGHT},
        {'b',Piece::BLACK_BISHOP},
        {'r',Piece::BLACK_ROOK},
        {'q',Piece::BLACK_QUEEN},
        {'k',Piece::BLACK_KING},
    };
    int idx = 0;
    for (char p : FEN){
        switch (p){
            case 'P':
            case 'N':
            case 'B':
            case 'R':
            case 'Q':
            case 'K':
            case 'p':
            case 'n':
            case 'b':
            case 'r':
            case 'q':
                (*this)[Piece_code_map[p]] |= 1LL << (63 - sq);
                sq++;
                break;
            case 'k':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
                sq += p - '0';
                break;
            case '/':
            case ' ':
                break;
        }
        if (p == ' '){
            break;
        }
        idx++;
    }

    idx++;
    if(FEN[idx] == 'w') (*this)[Piece::INFO] |= TURN_BIT;
        
    idx += 2;

    for(idx; FEN[idx] != ' '; idx++){
        switch (FEN[idx]){
            case 'K':
                (*this)[Piece::INFO] |= castling::WHITE_KINGSIDE_RIGHT;
                break;
            case 'Q':
                (*this)[Piece::INFO] |= castling::WHITE_QUEENSIDE_RIGHT;
                break;
            case 'k':
                (*this)[Piece::INFO] |= castling::BLACK_KINGSIDE_RIGHT;
                break;
            case 'q':
                (*this)[Piece::INFO] |= castling::BLACK_QUEENSIDE_RIGHT;
                break;
        }
    }

    idx++;

    if (FEN[idx] != '-'){
        char file = FEN[idx++];
        char rank = FEN[idx++];
        (*this)[Piece::INFO] |= sq_from_name(file,rank);
    }

    (*this)[Piece::WHITE_PCS] = 
        (*this)[Piece::WHITE_PAWN] |
        (*this)[Piece::WHITE_KNIGHT] |
        (*this)[Piece::WHITE_BISHOP] |
        (*this)[Piece::WHITE_ROOK] |
        (*this)[Piece::WHITE_QUEEN] |
        (*this)[Piece::WHITE_KING];
    
    (*this)[Piece::BLACK_PCS] = 
        (*this)[Piece::BLACK_PAWN] |
        (*this)[Piece::BLACK_KNIGHT] |
        (*this)[Piece::BLACK_BISHOP] |
        (*this)[Piece::BLACK_ROOK] |
        (*this)[Piece::BLACK_QUEEN] |
        (*this)[Piece::BLACK_KING];
    std::cout << "Bitboard constructor done\n";

}


void BitBoard::apply_move(const Move& m){

    (*this)[m.pc1] ^= m.mov1;
    (*this)[m.pc2] ^= m.mov2;
    (*this)[m.pc3] ^= m.mov3;
    
    (*this)[Piece::INFO] ^= m.info;
    
    (*this)[Piece::WHITE_PCS] = 
        (*this)[Piece::WHITE_PAWN] |
        (*this)[Piece::WHITE_KNIGHT] |
        (*this)[Piece::WHITE_BISHOP] |
        (*this)[Piece::WHITE_ROOK] |
        (*this)[Piece::WHITE_QUEEN] |
        (*this)[Piece::WHITE_KING]; 
    
    (*this)[Piece::BLACK_PCS] = 
        (*this)[Piece::BLACK_PAWN] |
        (*this)[Piece::BLACK_KNIGHT] |
        (*this)[Piece::BLACK_BISHOP] |
        (*this)[Piece::BLACK_ROOK] |
        (*this)[Piece::BLACK_QUEEN] |
        (*this)[Piece::BLACK_KING]; 

}