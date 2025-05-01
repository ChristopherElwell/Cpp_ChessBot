#pragma once
#include "bitboard.h"
#include "data.h"
#include <cstdint>
#include <unordered_map>

enum class movType : int{
    QUIET,
    CAPTURE,
    PROMOTE,
    CAPTURE_PROMOTE,
    CASTLE_KINGSIDE,
    CASTLE_QUEENSIDE,
    BOOK_END
};

constexpr int operator-(const movType a, const movType b){
    return static_cast<int>(a) - static_cast<int>(b);
}

class MoveGen{
    private:
        uint64_t get_white_rook_attacks(const BitBoard& board, const uint64_t rook);
        uint64_t get_black_rook_attacks(const BitBoard& board, const uint64_t rook);
        uint64_t get_white_bishop_attacks(const BitBoard& board, uint64_t bishop);
        uint64_t get_black_bishop_attacks(const BitBoard& board, uint64_t bishop);

        int get_white_knight_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board, int& idx);
        int get_black_knight_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board, int& idx);
        
        int get_white_bishop_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board, int& idx);
        int get_black_bishop_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board, int& idx);
        
        int get_white_rook_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board, int& idx);
        int get_black_rook_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board, int& idx);
        
        int get_white_pawn_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board, int& idx);
        int get_black_pawn_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board, int& idx);
        
        int get_white_queen_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board, int& idx);
        int get_black_queen_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board, int& idx);
        
        int get_white_king_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board, int& idx);
        int get_black_king_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board, int& idx);

        static int compare_moves(const Move& a, const Move& b);

    public:
        uint64_t get_white_attackers(const BitBoard& board);
        uint64_t get_black_attackers(const BitBoard& board);
        void get_white_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs,const BitBoard& board);
        void get_black_moves(std::array<Move,MOVES_ARRAY_LENGTH>& movs, const BitBoard& board);
};

