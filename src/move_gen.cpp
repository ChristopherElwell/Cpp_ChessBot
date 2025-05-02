#include "move_gen.h"
#include "move.h"
#include "data.h"
#include "bitboard.h"
#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <algorithm>

// get_moves.c does what it says, provides an array of legal moves given a position. get_white_moves, get_black_moves, get_white_attackers, and get_black_attackers are 
// the only functions called from other files. the first two are used to find legal moves in a position, the second two are used to get a mask of all squares that are attacked 
// by the opposing side to determine if the king is in check or if castling is legal (to be optimized)
// the search function first allocates an arrays of Move structs an the stack then passes a pointer into those interfacing functions. Each Piece for that side is then 
// called to fill the array with its moves, passing in a movptr to each function that is iterated as the array is filled to keep track of the next empty spot

// GET [PIECE] ATTACKS
// used for pieces that have a complicated function for getting attacking squares (bishops, rooks, and queens basically, but queens can steal rook and bishop implementation)
uint64_t MoveGen::get_white_rook_attacks(const BitBoard& board, const uint64_t rook){
    // uses a lookup table of legal sliding rook moves to get legal moves in O(1) time
    // the bit postiions of the pieces in the way are read as an 8 bit integer (8 squares that a rook can move to/from horizontally and vertically)
    // masks::ANTI_DIAG mask is used to transpose the board state to calcualte vertical moves as if they were horiztonal
    int pos = __builtin_ctzll(rook);
    int rank = pos >> 3;
    int file = pos & 7;
    int base = pos & ~7;
    uint64_t pcs = board[Piece::WHITE_PCS] | board[Piece::BLACK_PCS];
    uint64_t attacks = (uint64_t)(data::SLIDING_MOVES[(((pcs >> (base + 1)) & 0x3F) << 3) + file]) << base;
    
    uint64_t file_isolated = pcs << (8 - file) & masks::FILE_H;
    uint64_t rotated = (file_isolated * masks::ANTI_DIAG) >> 56;
    int index = (rotated * 8 + (7-rank)) & 0x1ff;
    uint64_t moves_rotated = ((uint64_t)data::SLIDING_MOVES[index]) * masks::ANTI_DIAG;
    attacks |= (moves_rotated & masks::FILE_A) >> (7 - file); 

    return attacks & ~board[Piece::WHITE_PCS];
}

uint64_t MoveGen::get_black_rook_attacks(const BitBoard& board, uint64_t rook){
    // see get_white_rook_attacks
    int pos = __builtin_ctzll(rook);
    int rank = pos >> 3;
    int file = pos & 7;
    int base = pos & ~7;
    uint64_t pcs = board[Piece::WHITE_PCS] | board[Piece::BLACK_PCS];

    uint64_t attacks = (uint64_t)(data::SLIDING_MOVES[(((pcs >> (base + 1)) & 0x3F) << 3) + file]) << base;
    
    uint64_t file_isolated = pcs << (8 - file) & masks::FILE_H;
    uint64_t rotated = (file_isolated * masks::ANTI_DIAG) >> 56;
    int index = (rotated * 8 + (7-rank)) & 0x1ff;
    uint64_t moves_rotated = ((uint64_t)data::SLIDING_MOVES[index]) * masks::ANTI_DIAG;
    attacks |= (moves_rotated & masks::FILE_A) >> (7 - file); 

    return attacks & ~board[Piece::BLACK_PCS];
}

uint64_t MoveGen::get_white_bishop_attacks(const BitBoard& board, uint64_t bishop){
    // in the future i want to implement a similar lookup table strategy as the rook moves
    // for now, uses many bitwise operations to calculate bishop moves in O(1) time
    
    uint64_t up, down, mask, temp, first_pc, spots;
    uint64_t whites = board[Piece::WHITE_PCS];
    uint64_t blacks = board[Piece::BLACK_PCS];
    uint64_t pcs = whites | blacks;
    int pos = __builtin_ctzll(bishop);
    up = masks::DIAGS_UP[(pos&7) + (pos>>3)]; // lookup tables to get up and right diagnoal moves (assuming empty board for now)
    down = masks::DIAGS_DOWN[7 + (pos>>3) - (pos&7)]; // same for down and right
    
    uint64_t sqs_ahead = ~((bishop - 1) | bishop); // all squares ahead of the bishop (as in all squares you would see before the bishop if reading the board like a book)
    uint64_t pcs_ahead = pcs & sqs_ahead; // all pcs ahead of the bishop
    uint64_t pcs_behind = pcs & (bishop - 1); // all pcs behind the bishop
    
    // up and right
    first_pc = pcs_ahead & -(pcs_ahead & up) & up;
    spots = ((first_pc - 1) & sqs_ahead & up) | (first_pc & blacks);
    
    // down and left
    temp = pcs_behind & up;
    mask = (!temp) - 1;
    first_pc = (A8 >> __builtin_clzll(temp)) & mask;
    spots |= (bishop - 1) & ~((first_pc - 1) | first_pc) & up;
    spots |= first_pc & blacks;
    spots |= up & (bishop - 1) & ~mask;

    // up and left
    first_pc = pcs_ahead & -(pcs_ahead & down) & down;
    spots |= ((first_pc - 1) & sqs_ahead & down) | (first_pc & blacks);
    
    // down and right
    temp = pcs_behind & down;
    mask = (!temp) - 1;
    first_pc = (A8 >> __builtin_clzll(temp)) & mask;
    spots |= (bishop - 1) & ~((first_pc - 1) | first_pc) & down;
    spots |= first_pc & blacks;
    spots |= down & (bishop - 1) & ~mask;
    return spots;
}

uint64_t MoveGen::get_black_bishop_attacks(const BitBoard& board, uint64_t bishop){
    // see get_white_bishop_attacks
    uint64_t up, down, mask, temp, first_pc, spots;
    uint64_t whites = board[Piece::WHITE_PCS];
    uint64_t blacks = board[Piece::BLACK_PCS];
    uint64_t pcs = whites | blacks;
    int pos = __builtin_ctzll(bishop);
    up = masks::DIAGS_UP[(pos&7) + (pos>>3)];
    down = masks::DIAGS_DOWN[7 + (pos>>3) - (pos&7)];
    
    uint64_t sqs_ahead = ~((bishop - 1) | bishop);
    uint64_t pcs_ahead = pcs & sqs_ahead;
    uint64_t pcs_behind = pcs & (bishop - 1);
    
    // up and right
    first_pc = pcs_ahead & -(pcs_ahead & up) & up;
    spots = ((first_pc - 1) & sqs_ahead & up) | (first_pc & whites);
    
    // down and left
    temp = pcs_behind & up;
    mask = (!temp) - 1;
    first_pc = (A8 >> __builtin_clzll(temp)) & mask;
    spots |= (bishop - 1) & ~((first_pc - 1) | first_pc) & up;
    spots |= first_pc & whites;
    spots |= up & (bishop - 1) & ~mask;

    // up and left
    first_pc = pcs_ahead & -(pcs_ahead & down) & down;
    spots |= ((first_pc - 1) & sqs_ahead & down) | (first_pc & whites);
    
    // down and right
    temp = pcs_behind & down;
    mask = (!temp) - 1;
    first_pc = (A8 >> __builtin_clzll(temp)) & mask;
    spots |= (bishop - 1) & ~((first_pc - 1) | first_pc) & down;
    spots |= first_pc & whites;
    spots |= down & (bishop - 1) & ~mask;
    return spots;
}

// GET ATTACKS
// Used to determine if king is in check or if castling is legal

// GET LEGAL [PIECE] MOVES
// similar implementation for each, iterates over all legal moves, seperating capturing moves, promoting moves, empty moves, and capturing and promoting moves
// fills mov array by calling create_move functions while incrementing movptr to the next empty spot

int MoveGen::get_white_knight_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board, int& idx){

    const uint64_t whites = board[Piece::WHITE_PCS];
    const uint64_t blacks = board[Piece::BLACK_PCS];

    uint64_t knights = board[Piece::WHITE_KNIGHT];
    uint64_t knight, taking_move, taken_piece;

    for (; knights; knights &= (knights - 1)){
        knight = knights & -knights;
        uint64_t moves = data::KNIGHT_MOVES[__builtin_ctzll(knight)] & ~whites;
        for (uint64_t clear_moves = moves & ~blacks; clear_moves; clear_moves &= (clear_moves - 1)){
            uint64_t clear_move = (clear_moves & -clear_moves) | knight;
            movs[idx++] = Move::quiet(Piece::WHITE_KNIGHT,clear_move,0,board[Piece::INFO]); 
        }
        for (uint64_t taking_moves = moves & blacks; taking_moves; taking_moves &= (taking_moves - 1)){
            taking_move = (taking_moves & -taking_moves) | knight;
            for (Piece pc : PieceRange::BlackNoKing()){
                if ((taken_piece = (taking_move & board[pc]))){
                    movs[idx++] = Move::capture(Piece::WHITE_KNIGHT,taking_move,pc,taken_piece,0,board[Piece::INFO]);
                    break;
                }
            }        
        }
    }
    return idx;
}

int MoveGen::get_black_knight_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs, const BitBoard& board, int& idx){
    const uint64_t whites = board[Piece::WHITE_PCS];
    const uint64_t blacks = board[Piece::BLACK_PCS];
    uint64_t knights = board[Piece::BLACK_KNIGHT];
    uint64_t knight, taking_move, taken_piece;

    for (; knights; knights &= (knights - 1)){
        knight = knights & -knights;
        uint64_t moves = data::KNIGHT_MOVES[__builtin_ctzll(knight)] & ~blacks;
        for (uint64_t clear_moves = moves & ~whites; clear_moves; clear_moves &= (clear_moves - 1)){
            uint64_t clear_move = (clear_moves & (~clear_moves + 1)) | knight;
            movs[idx++] = Move::quiet(Piece::BLACK_KNIGHT,clear_move,0,board[Piece::INFO]); 
        }
        for (uint64_t taking_moves = moves & whites; taking_moves; taking_moves &= (taking_moves - 1)){
            taking_move = (taking_moves & (~taking_moves + 1)) | knight;
            for (Piece pc : PieceRange::WhiteNoKing()){
                if ((taken_piece = (taking_move & board[pc]))){
                    movs[idx++] = Move::capture(Piece::BLACK_KNIGHT,taking_move,pc,taken_piece,0,board[Piece::INFO]);
                    break;
                }
            }        
        }
    }
    return idx;
}

int MoveGen::get_white_rook_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs, const BitBoard& board, int& idx){
    uint64_t rooks = board[Piece::WHITE_ROOK];

    uint64_t rook, spot, taken_piece; 

    for(;rooks; rooks &= (rooks - 1)){
        rook = rooks & -rooks;
        uint64_t moves = get_white_rook_attacks(board,rook);
        uint64_t spots = moves & ~board[Piece::BLACK_PCS];
        uint64_t taking_spots = moves & board[Piece::BLACK_PCS];
        // empty moves
        uint64_t castling_rights_info_xor = board[Piece::INFO] & rook & (castling::WHITE_KINGSIDE_RIGHT | castling::WHITE_QUEENSIDE_RIGHT);
        for(; spots; spots &= (spots - 1)){
            spot = (spots & (~spots + 1)) | rook;
            movs[idx++] = Move::quiet(Piece::WHITE_ROOK,spot,castling_rights_info_xor,board[Piece::INFO]);
        }
        
        // taking moves
        for(;taking_spots; taking_spots &= (taking_spots - 1)){
            spot = (taking_spots & (~taking_spots + 1)) | rook;
            for (Piece pc : PieceRange::BlackNoKing()){
                if ((taken_piece = (spot & board[pc]))){
                    movs[idx++] = Move::capture(Piece::WHITE_ROOK,spot,pc,taken_piece,castling_rights_info_xor,board[Piece::INFO]);
                    break;
                }
            }
        }
    }
    return idx;
}

int MoveGen::get_black_rook_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs, const BitBoard& board, int& idx){
    uint64_t rooks = board[Piece::BLACK_ROOK];

    uint64_t rook, spot, taken_piece; 

    for(;rooks; rooks &= (rooks - 1)){
        rook = rooks & -rooks;
        uint64_t moves = get_black_rook_attacks(board,rook);
        uint64_t castling_rights_info_xor = board[Piece::INFO] & rook & (castling::BLACK_KINGSIDE_RIGHT | castling::BLACK_QUEENSIDE_RIGHT); 
        // empty moves
        for(uint64_t spots = moves & ~board[Piece::WHITE_PCS]; spots; spots &= (spots - 1)){
            spot = (spots & (~spots + 1)) | rook;
            movs[idx++] = Move::quiet(Piece::BLACK_ROOK,spot,castling_rights_info_xor,board[Piece::INFO]);
        }
        
        // taking moves
        for(uint64_t taking_spots = moves & board[Piece::WHITE_PCS]; taking_spots; taking_spots &= (taking_spots - 1)){
            spot = (taking_spots & (~taking_spots + 1)) | rook;
            for (Piece pc : PieceRange::WhiteNoKing()){
                if ((taken_piece = (spot & board[pc]))){
                    movs[idx++] = Move::capture(Piece::BLACK_ROOK,spot,pc,taken_piece,castling_rights_info_xor,board[Piece::INFO]);
                    break;
                }
            }
        }
    }
    return idx;
}

int MoveGen::get_white_bishop_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs, const BitBoard& board, int& idx){
    const uint64_t blacks = board[Piece::BLACK_PCS];
    uint64_t bishops = board[Piece::WHITE_BISHOP];
    uint64_t bishop, spot, taken_piece;
    
    for (; bishops; bishops &= (bishops - 1)){
        bishop = bishops & -bishops;
        const uint64_t moves = get_white_bishop_attacks(board,bishop);
        // empty moves
        for(uint64_t spots = moves & ~blacks; spots; spots &= (spots - 1)){
            spot = (spots & (~spots + 1)) | bishop;
            movs[idx++] = Move::quiet(Piece::WHITE_BISHOP,spot,0,board[Piece::INFO]);
        }
        
        // taking moves
        for(uint64_t taking_spots = moves & blacks; taking_spots; taking_spots &= (taking_spots - 1)){
            spot = (taking_spots & (~taking_spots + 1)) | bishop;
            for (Piece pc : PieceRange::BlackNoKing()){
                if ((taken_piece = (spot & board[pc]))){
                    movs[idx++] = Move::capture(Piece::WHITE_BISHOP,spot,pc,taken_piece,0,board[Piece::INFO]);
                    break;
                }
            }
        }
    }
    return idx;
}

int MoveGen::get_black_bishop_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board, int& idx){
    const uint64_t whites = board[Piece::WHITE_PCS];
    uint64_t bishops = board[Piece::BLACK_BISHOP];

    uint64_t bishop, spot, taken_piece;
    
    for (; bishops; bishops &= (bishops - 1)){
        bishop = bishops & -bishops;
        const uint64_t moves = get_black_bishop_attacks(board,bishop);
        
        for(uint64_t spots = moves & ~whites; spots; spots &= (spots - 1)){
            spot = (spots & (~spots + 1)) | bishop;
            movs[idx++] = Move::quiet(Piece::BLACK_BISHOP,spot,0,board[Piece::INFO]);
        }
        // taking moves
        for(uint64_t taking_spots = moves & whites; taking_spots; taking_spots &= (taking_spots - 1)){
            spot = (taking_spots & (~taking_spots + 1)) | bishop;
            for (Piece pc : PieceRange::WhiteNoKing()){
                if ((taken_piece = (spot & board[pc]))){
                    movs[idx++] = Move::capture(Piece::BLACK_BISHOP,spot,pc,taken_piece,0,board[Piece::INFO]);
                    break;
                }
            }
        }
    }
    return idx;
}

int MoveGen::get_white_pawn_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board, int& idx){
    const uint64_t whites = board[Piece::WHITE_PCS];
    const uint64_t blacks = board[Piece::BLACK_PCS];
    uint64_t pawns = board[Piece::WHITE_PAWN];
    uint64_t pcs = whites | blacks;
    uint64_t spot, taken_piece, promotion_sq;

    // step forward one
    for (uint64_t one_step = (pawns << 8) & ~pcs; one_step; one_step &= (one_step - 1)){
        spot = (one_step & (~one_step + 1));
        spot |= spot >> 8;
        if ((promotion_sq = (spot & masks::RANK_8))){
            movs[idx++] = Move::promote(Piece::WHITE_PAWN,spot & ~promotion_sq,Piece::WHITE_QUEEN,promotion_sq,0,board[Piece::INFO]);
            movs[idx++] = Move::promote(Piece::WHITE_PAWN,spot & ~promotion_sq,Piece::WHITE_KNIGHT,promotion_sq,0,board[Piece::INFO]);
            movs[idx++] = Move::promote(Piece::WHITE_PAWN,spot & ~promotion_sq,Piece::WHITE_ROOK,promotion_sq,0,board[Piece::INFO]);
            movs[idx++] = Move::promote(Piece::WHITE_PAWN,spot & ~promotion_sq,Piece::WHITE_BISHOP,promotion_sq,0,board[Piece::INFO]);
        } else {
            movs[idx++] = Move::quiet(Piece::WHITE_PAWN,spot,0,board[Piece::INFO]);
        }
    }
    // step forward two
    for (uint64_t two_step = ((pawns & masks::RANK_2) << 16) & ~((pcs) | (pcs << 8)); two_step; two_step &= (two_step - 1)){
        spot = (two_step & (~two_step + 1));
        movs[idx++] = Move::quiet(Piece::WHITE_PAWN,spot | spot >> 16,(spot >> 8),board[Piece::INFO]);
    }

    // taking right
    for (uint64_t take_right = (pawns << 7) & blacks & ~masks::FILE_A; take_right; take_right &= (take_right - 1)){
        spot = (take_right & (~take_right + 1));
        spot |= spot >> 7;
        for (Piece pc : PieceRange::BlackNoKing()){
            if ((taken_piece = (spot & board[pc]))){
                if ((promotion_sq = (spot & masks::RANK_8))){
                    movs[idx++] = Move::promote_capture(Piece::WHITE_PAWN,spot & ~promotion_sq,pc,taken_piece,Piece::WHITE_QUEEN,promotion_sq,0,board[Piece::INFO]);
                    movs[idx++] = Move::promote_capture(Piece::WHITE_PAWN,spot & ~promotion_sq,pc,taken_piece,Piece::WHITE_KNIGHT,promotion_sq,0,board[Piece::INFO]);
                    movs[idx++] = Move::promote_capture(Piece::WHITE_PAWN,spot & ~promotion_sq,pc,taken_piece,Piece::WHITE_ROOK,promotion_sq,0,board[Piece::INFO]);
                    movs[idx++] = Move::promote_capture(Piece::WHITE_PAWN,spot & ~promotion_sq,pc,taken_piece,Piece::WHITE_BISHOP,promotion_sq,0,board[Piece::INFO]);
                } else {
                    movs[idx++] = Move::capture(Piece::WHITE_PAWN,spot,pc,taken_piece,0,board[Piece::INFO]);
                }
                break;
            }
        }
    }
    // taking left
    for (uint64_t take_left = (pawns << 9) & blacks & ~masks::FILE_H; take_left; take_left &= (take_left - 1)){
        spot = (take_left & (~take_left + 1));
        spot |= spot >> 9;
        for (Piece pc : PieceRange::BlackNoKing()){
            if ((taken_piece = (spot & board[pc]))){
                if ((promotion_sq = (spot & masks::RANK_8))){
                    movs[idx++] = Move::promote_capture(Piece::WHITE_PAWN,spot & ~promotion_sq,pc,taken_piece,Piece::WHITE_QUEEN,promotion_sq,0,board[Piece::INFO]);
                    movs[idx++] = Move::promote_capture(Piece::WHITE_PAWN,spot & ~promotion_sq,pc,taken_piece,Piece::WHITE_KNIGHT,promotion_sq,0,board[Piece::INFO]);
                    movs[idx++] = Move::promote_capture(Piece::WHITE_PAWN,spot & ~promotion_sq,pc,taken_piece,Piece::WHITE_ROOK,promotion_sq,0,board[Piece::INFO]);
                    movs[idx++] = Move::promote_capture(Piece::WHITE_PAWN,spot & ~promotion_sq,pc,taken_piece,Piece::WHITE_BISHOP,promotion_sq,0,board[Piece::INFO]);
                } else {
                    movs[idx++] = Move::capture(Piece::WHITE_PAWN,spot,pc,taken_piece,0,board[Piece::INFO]);
                }
                break;
            }
        }
    }
    // taking en passent 
    uint64_t en_passent_take_left = ((pawns << 9) & board[Piece::INFO] & ~masks::RANK_1 & ~masks::FILE_H);
    if (en_passent_take_left){
        movs[idx++] = Move::capture(Piece::WHITE_PAWN,en_passent_take_left | (en_passent_take_left >> 9),
                                    Piece::BLACK_PAWN,en_passent_take_left >> 8,0,board[Piece::INFO]);
    }
    
    uint64_t en_passent_take_right = ((pawns << 7) & board[Piece::INFO] & ~masks::RANK_1 & ~masks::FILE_A);
    if (en_passent_take_right){
        movs[idx++] = Move::capture(Piece::WHITE_PAWN,en_passent_take_right | (en_passent_take_right >> 7),
                                    Piece::BLACK_PAWN,en_passent_take_right >> 8,0,board[Piece::INFO]);
    }
    return idx;
}

int MoveGen::get_black_pawn_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board, int& idx){
    const uint64_t whites = board[Piece::WHITE_PCS];
    const uint64_t blacks = board[Piece::BLACK_PCS];
    uint64_t pawns = board[Piece::BLACK_PAWN];
    uint64_t pcs = whites | blacks;
    uint64_t spot, taken_piece, promotion_sq;

    for (uint64_t one_step = (pawns >> 8) & ~pcs; one_step; one_step &= (one_step - 1)){
        spot = (one_step & (~one_step + 1));
        spot |= spot << 8;
        if ((promotion_sq = (spot & masks::RANK_1))){
            movs[idx++] = Move::promote(Piece::BLACK_PAWN,spot & ~promotion_sq,Piece::BLACK_QUEEN,promotion_sq,0,board[Piece::INFO]);
            movs[idx++] = Move::promote(Piece::BLACK_PAWN,spot & ~promotion_sq,Piece::BLACK_KNIGHT,promotion_sq,0,board[Piece::INFO]);
            movs[idx++] = Move::promote(Piece::BLACK_PAWN,spot & ~promotion_sq,Piece::BLACK_BISHOP,promotion_sq,0,board[Piece::INFO]);
            movs[idx++] = Move::promote(Piece::BLACK_PAWN,spot & ~promotion_sq,Piece::BLACK_ROOK,promotion_sq,0,board[Piece::INFO]);
        } else {
            movs[idx++] = Move::quiet(Piece::BLACK_PAWN,spot,0,board[Piece::INFO]);
        }
    }
    for (uint64_t two_step = ((pawns & masks::RANK_7) >> 16) & ~((pcs) | (pcs >> 8)); two_step; two_step &= (two_step - 1)){
        spot = (two_step & (~two_step + 1));
        // spot |= spot << 16;
        movs[idx++] = Move::quiet(Piece::BLACK_PAWN,spot | spot << 16,(spot << 8),board[Piece::INFO]);
    }
    for (uint64_t take_right = (pawns >> 9) & whites & ~masks::FILE_A; take_right; take_right &= (take_right - 1)){
        spot = (take_right & (~take_right + 1));
        spot |= spot << 9;
        for (Piece pc : PieceRange::WhiteNoKing()){
            if ((taken_piece = (spot & board[pc]))){
                if ((promotion_sq = (spot & masks::RANK_1))){
                    movs[idx++] = Move::promote_capture(Piece::BLACK_PAWN,spot & ~promotion_sq,pc,taken_piece,Piece::BLACK_QUEEN,promotion_sq,0,board[Piece::INFO]);
                    movs[idx++] = Move::promote_capture(Piece::BLACK_PAWN,spot & ~promotion_sq,pc,taken_piece,Piece::BLACK_KNIGHT,promotion_sq,0,board[Piece::INFO]);
                    movs[idx++] = Move::promote_capture(Piece::BLACK_PAWN,spot & ~promotion_sq,pc,taken_piece,Piece::BLACK_BISHOP,promotion_sq,0,board[Piece::INFO]);
                    movs[idx++] = Move::promote_capture(Piece::BLACK_PAWN,spot & ~promotion_sq,pc,taken_piece,Piece::BLACK_ROOK,promotion_sq,0,board[Piece::INFO]);
                } else {
                    movs[idx++] = Move::capture(Piece::BLACK_PAWN,spot,pc,taken_piece,0,board[Piece::INFO]);
                }
                break;
            }
        }
    }
    for (uint64_t take_left = (pawns >> 7) & whites & ~masks::FILE_H; take_left; take_left &= (take_left - 1)){
        spot = (take_left & (~take_left + 1));
        spot |= spot << 7;
        for (Piece pc : PieceRange::WhiteNoKing()){
            if ((taken_piece = (spot & board[pc]))){
                if ((promotion_sq = (spot & masks::RANK_1))){
                    movs[idx++] = Move::promote_capture(Piece::BLACK_PAWN,spot & ~promotion_sq,pc,taken_piece,Piece::BLACK_QUEEN,promotion_sq,0,board[Piece::INFO]);
                    movs[idx++] = Move::promote_capture(Piece::BLACK_PAWN,spot & ~promotion_sq,pc,taken_piece,Piece::BLACK_KNIGHT,promotion_sq,0,board[Piece::INFO]);
                    movs[idx++] = Move::promote_capture(Piece::BLACK_PAWN,spot & ~promotion_sq,pc,taken_piece,Piece::BLACK_BISHOP,promotion_sq,0,board[Piece::INFO]);
                    movs[idx++] = Move::promote_capture(Piece::BLACK_PAWN,spot & ~promotion_sq,pc,taken_piece,Piece::BLACK_ROOK,promotion_sq,0,board[Piece::INFO]);
                } else {
                    movs[idx++] = Move::capture(Piece::BLACK_PAWN,spot,pc,taken_piece,0,board[Piece::INFO]);
                }
                break;
            }
        }
    }

    // taking en passent 
    uint64_t en_passent_take_left = ((pawns >> 7) & board[Piece::INFO] & ~masks::RANK_1 & ~masks::FILE_H);
    if (en_passent_take_left){
        movs[idx++] = Move::capture(Piece::BLACK_PAWN,en_passent_take_left | (en_passent_take_left << 7),
                                Piece::WHITE_PAWN,en_passent_take_left << 8,0,board[Piece::INFO]);
    }
    
    uint64_t en_passent_take_right = ((pawns >> 9) & board[Piece::INFO] & ~masks::RANK_1 & ~masks::FILE_A);
    if (en_passent_take_right){
        movs[idx++] = Move::capture(Piece::BLACK_PAWN,en_passent_take_right | (en_passent_take_right << 9),
                                Piece::WHITE_PAWN,en_passent_take_right << 8,0,board[Piece::INFO]);
    }
    return idx;
}

int MoveGen::get_white_king_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board, int& idx){
    const uint64_t whites = board[Piece::WHITE_PCS];
    const uint64_t blacks = board[Piece::BLACK_PCS];
    uint64_t king = board[Piece::WHITE_KING];
    uint64_t moves = data::KING_MOVES[__builtin_ctzll(king)] & ~whites;
    uint64_t move, taken_piece;

    const uint64_t info_xor = (castling::WHITE_KINGSIDE_RIGHT | castling::WHITE_QUEENSIDE_RIGHT) & board[Piece::INFO];

    for(uint64_t clear_moves = moves & ~blacks; clear_moves; clear_moves &= (clear_moves - 1)){
        move = (clear_moves & (-clear_moves)) | king;
        movs[idx++] = Move::quiet(Piece::WHITE_KING, move, info_xor,board[Piece::INFO]);
    }
    
    for(uint64_t taking_moves = moves & blacks; taking_moves; taking_moves &= (taking_moves - 1)){
        move = (taking_moves & (-taking_moves)) | king;
        for (Piece pc : PieceRange::BlackNoKing()){
            if ((taken_piece = board[pc] & move)){
                movs[idx++] = Move::capture(Piece::WHITE_KING,move,pc,taken_piece,info_xor,board[Piece::INFO]);
            }           
        }        
    } 
    uint64_t attacks = 0;
    if ((board[Piece::INFO] & castling::WHITE_KINGSIDE_RIGHT) && 
    ((castling::WHITE_KINGSIDE_SPACE & (whites | blacks)) == 0) &&
    (board[Piece::WHITE_ROOK] & masks::FILE_H & masks::RANK_1)){
        attacks = get_black_attackers(board);
        if ((attacks & castling::WHITE_KINGSIDE_ATTACKED) == 0){
            movs[idx++] = Move::castle_kingside(Piece::WHITE_KING,0b1010ULL,Piece::WHITE_ROOK,0b0101ULL,info_xor,board[Piece::INFO]);
        }
    }
    if ((board[Piece::INFO] & castling::WHITE_QUEENSIDE_RIGHT) && 
        ((castling::WHITE_QUEENSIDE_SPACE & (whites | blacks)) == 0) &&
        ((board[Piece::WHITE_ROOK] & masks::FILE_A & masks::RANK_1))){
            if (attacks == 0) attacks = get_black_attackers(board);
            
            if ((attacks & castling::WHITE_QUEENSIDE_ATTACKED) == 0){
                movs[idx++] = Move::castle_queenside(Piece::WHITE_KING,0b101000ULL,Piece::WHITE_ROOK,0b10010000ULL,info_xor,board[Piece::INFO]);
            }
    }
    return idx;
}

int MoveGen::get_black_king_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board, int& idx){
    const uint64_t whites = board[Piece::WHITE_PCS];
    const uint64_t blacks = board[Piece::BLACK_PCS];
    uint64_t king = board[Piece::BLACK_KING];
    uint64_t moves = data::KING_MOVES[__builtin_ctzll(king)] & ~blacks;
    uint64_t move, taken_piece;


    const uint64_t info_xor =  (castling::BLACK_KINGSIDE_RIGHT | castling::BLACK_QUEENSIDE_RIGHT) & board[Piece::INFO];

    for(uint64_t clear_moves = moves & ~whites; clear_moves; clear_moves &= clear_moves - 1){
        move = (clear_moves & (-clear_moves)) | king;
        movs[idx++] = Move::quiet(Piece::BLACK_KING,move,info_xor,board[Piece::INFO]);
    }

    for(uint64_t taking_moves = moves & whites; taking_moves; taking_moves &= taking_moves - 1){
        move = (taking_moves & (-taking_moves)) | king;
        for (Piece pc : PieceRange::WhiteNoKing()){
            if ((taken_piece = board[pc] & move)){
                movs[idx++] = Move::capture(Piece::BLACK_KING,move,pc,taken_piece,info_xor,board[Piece::INFO]);
            }
        }        
    }
    uint64_t attacks = 0;
    if ((board[Piece::INFO] & castling::BLACK_KINGSIDE_RIGHT) && 
        ((castling::BLACK_KINGSIDE_SPACE & (whites | blacks)) == 0) &&
        (board[Piece::BLACK_ROOK] & masks::FILE_H & masks::RANK_8)){
            attacks = get_white_attackers(board);
            if ((attacks & castling::BLACK_KINGSIDE_ATTACKED) == 0){
                movs[idx++] = Move::castle_kingside(Piece::BLACK_KING,0b1010ULL << 56,Piece::BLACK_ROOK,0b0101ULL << 56,info_xor,board[Piece::INFO]);
            }
    }

    if ((board[Piece::INFO] & castling::BLACK_QUEENSIDE_RIGHT) && 
        ((castling::BLACK_QUEENSIDE_SPACE & (whites | blacks)) == 0) &&
        (board[Piece::BLACK_ROOK] & masks::FILE_A & masks::RANK_8)){
            if (attacks == 0) attacks = get_white_attackers(board);
            
            if ((attacks & castling::BLACK_QUEENSIDE_ATTACKED) == 0){
                movs[idx++] = Move::castle_queenside(Piece::BLACK_KING,0b101000ULL << 56,Piece::BLACK_ROOK,0b10010000ULL << 56,info_xor,board[Piece::INFO]);
            }
    }
    return idx;
}

int MoveGen::get_white_queen_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs, const BitBoard& board, int& idx){
    const uint64_t blacks = board[Piece::BLACK_PCS];
    uint64_t queens = board[Piece::WHITE_QUEEN];
    uint64_t queen, spot, taken_piece;
    
    for (; queens; queens &= (queens - 1)){
        queen = queens & -queens;
        const uint64_t moves = get_white_bishop_attacks(board,queen) | get_white_rook_attacks(board,queen);
        
        for(uint64_t spots = moves & ~blacks; spots; spots &= (spots - 1)){
            spot = (spots & (~spots + 1)) | queen;
            movs[idx++] = Move::quiet(Piece::WHITE_QUEEN,spot,0,board[Piece::INFO]);
        }
        // taking moves
        for(uint64_t taking_spots = moves & blacks; taking_spots; taking_spots &= (taking_spots - 1)){
            spot = (taking_spots & (~taking_spots + 1)) | queen;
            for (Piece pc : PieceRange::BlackNoKing()){
                if ((taken_piece = (spot & board[pc]))){
                    movs[idx++] = Move::capture(Piece::WHITE_QUEEN,spot,pc,taken_piece,0,board[Piece::INFO]);
                    break;
                }
            }
        }
    }
    return idx;
}

int MoveGen::get_black_queen_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs, const BitBoard& board, int& idx){
    const uint64_t whites = board[Piece::WHITE_PCS];
    uint64_t queens = board[Piece::BLACK_QUEEN];
    uint64_t queen, spot, taken_piece;

    for (; queens; queens &= (queens - 1)){
        queen = queens & -queens;
        const uint64_t moves = get_black_bishop_attacks(board,queen) | get_black_rook_attacks(board,queen);
        
        for(uint64_t spots = moves & ~whites; spots; spots &= (spots - 1)){
            spot = (spots & (~spots + 1)) | queen;
            movs[idx++] = Move::quiet(Piece::BLACK_QUEEN,spot,0,board[Piece::INFO]);
        }
        // taking moves
        for(uint64_t taking_spots = moves & whites; taking_spots; taking_spots &= (taking_spots - 1)){
            spot = (taking_spots & (~taking_spots + 1)) | queen;
            for (Piece pc : PieceRange::WhiteNoKing()){
                if ((taken_piece = (spot & board[pc]))){
                    movs[idx++] = Move::capture(Piece::BLACK_QUEEN,spot,pc,taken_piece,0,board[Piece::INFO]);
                    break;
                }
            }
        }
    }
    return idx;
}

// GET LEGAL MOVES
// used to get all legal moves given a board position

uint64_t MoveGen::get_white_attackers(const BitBoard& board){
    uint64_t attacks = 0;
    attacks |= ((board[Piece::WHITE_PAWN] & ~masks::FILE_H) << 7) | ((board[Piece::WHITE_PAWN] & ~masks::FILE_A) << 9);

    for (uint64_t pcs = board[Piece::WHITE_KING]; pcs; pcs &= (pcs - 1)){
        attacks |= data::KING_MOVES[__builtin_ctzll(pcs & -pcs)];
    }
    
    for (uint64_t pcs = board[Piece::WHITE_KNIGHT]; pcs; pcs &= (pcs - 1)){
        attacks |= data::KNIGHT_MOVES[__builtin_ctzll(pcs & -pcs)];
    }
    
    for (uint64_t pcs = (board[Piece::WHITE_BISHOP] | board[Piece::WHITE_QUEEN]); pcs; pcs &= (pcs - 1)){
        attacks |= get_white_bishop_attacks(board,pcs & -pcs);
    }
    
    for (uint64_t pcs = (board[Piece::WHITE_ROOK] | board[Piece::WHITE_QUEEN]); pcs; pcs &= (pcs - 1)){
        attacks |= get_white_rook_attacks(board,pcs & -pcs);
    }

    return attacks;
}

uint64_t MoveGen::get_black_attackers(const BitBoard& board){

    uint64_t attacks = 0;
    
    attacks |= ((board[Piece::BLACK_PAWN] & ~masks::FILE_H) >> 9) | ((board[Piece::BLACK_PAWN] & ~masks::FILE_A) >> 7);
    
    for (uint64_t pcs = board[Piece::BLACK_KING]; pcs; pcs &= (pcs - 1)){
        attacks |= data::KING_MOVES[__builtin_ctzll(pcs & -pcs)];
            }
    
    for (uint64_t pcs = board[Piece::BLACK_KNIGHT]; pcs; pcs &= (pcs - 1)){
        attacks |= data::KNIGHT_MOVES[__builtin_ctzll(pcs & -pcs)];
    }
    
    for (uint64_t pcs = (board[Piece::BLACK_BISHOP] | board[Piece::BLACK_QUEEN]); pcs; pcs &= (pcs - 1)){
        attacks |= get_black_bishop_attacks(board,pcs & -pcs);
    }
    
    for (uint64_t pcs = (board[Piece::BLACK_ROOK] | board[Piece::BLACK_QUEEN]); pcs; pcs &= (pcs - 1)){
        attacks |= get_black_rook_attacks(board,pcs & -pcs);
        // print_bit_board(get_black_rook_attacks(board,pcs & -pcs));
    }

    return attacks;
}

// white side interfacing function
void MoveGen::get_white_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board){
    int idx = 0;
    idx = get_white_queen_moves(movs,board,idx);
    idx = get_white_rook_moves(movs,board,idx);
    idx = get_white_bishop_moves(movs,board,idx);
    idx = get_white_knight_moves(movs,board,idx);
    idx = get_white_pawn_moves(movs,board,idx);
    idx = get_white_king_moves(movs,board,idx);
    movs[idx].type = movType::BOOK_END;

    // apply move ordering
    std::sort(movs.begin(), movs.begin() + idx, compare_moves);
}

// black side interfacing function
void MoveGen::get_black_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs, const BitBoard& board){
    int idx = 0;
    idx = get_black_queen_moves(movs,board,idx);
    idx = get_black_rook_moves(movs,board,idx);
    idx = get_black_bishop_moves(movs,board,idx);
    idx = get_black_knight_moves(movs,board,idx);
    idx = get_black_pawn_moves(movs,board,idx);
    idx = get_black_king_moves(movs,board,idx);
    movs[idx].type = movType::BOOK_END;
    
    // apply move ordering
    std::sort(movs.begin(), movs.begin() + idx, compare_moves);
}