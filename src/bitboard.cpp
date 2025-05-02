#include "bitboard.h"
#include "data.h"
#include "move.h"
#include "move_gen.h"
#include "helpers.h"
#include <cstdint>
#include <array>
#include <unordered_map>

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
            case 'k':
                board[static_cast<int>(Piece_code_map[p])] |= 1LL << (63 - sq);
                sq++;
                break;
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
    if(FEN[idx] == 'w') board[static_cast<int>(Piece::INFO)] |= TURN_BIT;
        
    idx += 2;

    for(; FEN[idx] != ' '; idx++){
        switch (FEN[idx]){
            case 'K':
                board[static_cast<int>(Piece::INFO)] |= castling::WHITE_KINGSIDE_RIGHT;
                break;
            case 'Q':
                board[static_cast<int>(Piece::INFO)] |= castling::WHITE_QUEENSIDE_RIGHT;
                break;
            case 'k':
                board[static_cast<int>(Piece::INFO)] |= castling::BLACK_KINGSIDE_RIGHT;
                break;
            case 'q':
                board[static_cast<int>(Piece::INFO)] |= castling::BLACK_QUEENSIDE_RIGHT;
                break;
        }
    }

    idx++;

    if (FEN[idx] != '-'){
        char file = FEN[idx++];
        char rank = FEN[idx++];
        board[static_cast<int>(Piece::INFO)] |= helpers::sq_from_name(file,rank);
    }

    board[static_cast<int>(Piece::WHITE_PCS)] = 
        board[static_cast<int>(Piece::WHITE_PAWN)] |
        board[static_cast<int>(Piece::WHITE_KNIGHT)] |
        board[static_cast<int>(Piece::WHITE_BISHOP)] |
        board[static_cast<int>(Piece::WHITE_ROOK)] |
        board[static_cast<int>(Piece::WHITE_QUEEN)] |
        board[static_cast<int>(Piece::WHITE_KING)];
    
    board[static_cast<int>(Piece::BLACK_PCS)] = 
        board[static_cast<int>(Piece::BLACK_PAWN)] |
        board[static_cast<int>(Piece::BLACK_KNIGHT)] |
        board[static_cast<int>(Piece::BLACK_BISHOP)] |
        board[static_cast<int>(Piece::BLACK_ROOK)] |
        board[static_cast<int>(Piece::BLACK_QUEEN)] |
        board[static_cast<int>(Piece::BLACK_KING)];
}

void BitBoard::apply_move(const Move& m){

    board[static_cast<int>(m.pc1)] ^= m.mov1;
    board[static_cast<int>(m.pc2)] ^= m.mov2;
    board[static_cast<int>(m.pc3)] ^= m.mov3;
    
    board[static_cast<int>(Piece::INFO)] ^= m.info;
    
    board[static_cast<int>(Piece::WHITE_PCS)] = 
        board[static_cast<int>(Piece::WHITE_PAWN)] |
        board[static_cast<int>(Piece::WHITE_KNIGHT)] |
        board[static_cast<int>(Piece::WHITE_BISHOP)] |
        board[static_cast<int>(Piece::WHITE_ROOK)] |
        board[static_cast<int>(Piece::WHITE_QUEEN)] |
        board[static_cast<int>(Piece::WHITE_KING)]; 
    
    board[static_cast<int>(Piece::BLACK_PCS)] = 
        board[static_cast<int>(Piece::BLACK_PAWN)] |
        board[static_cast<int>(Piece::BLACK_KNIGHT)] |
        board[static_cast<int>(Piece::BLACK_BISHOP)] |
        board[static_cast<int>(Piece::BLACK_ROOK)] |
        board[static_cast<int>(Piece::BLACK_QUEEN)] |
        board[static_cast<int>(Piece::BLACK_KING)]; 

}