#pragma once
#include <array>
#include <cstdint>
#include <string>

struct Move;

enum class Piece : int{
    WHITE_PAWN,
    WHITE_KNIGHT,
    WHITE_BISHOP,
    WHITE_ROOK,
    WHITE_QUEEN,
    WHITE_KING,
    BLACK_PAWN,
    BLACK_KNIGHT,
    BLACK_BISHOP,
    BLACK_ROOK,
    BLACK_QUEEN,
    BLACK_KING,
    WHITE_PCS,
    BLACK_PCS,
    INFO,
    PIECECOUNT
};

constexpr int operator-(const Piece a, const Piece b){
    return static_cast<int>(a) - static_cast<int>(b);
}

struct PieceRange {
    public:
        struct Iterator {
            int value;
            Piece operator*() const {return static_cast<Piece>(value); }
            Iterator& operator++() { ++value; return *this; }
            bool operator!=(const Iterator& other) const {return value != other.value; }
        };

        Iterator begin() const { return {static_cast<int>(start)}; }
        Iterator end() const { return {static_cast<int>(stop) + 1}; }

        PieceRange(Piece start, Piece stop) : start(start), stop(stop) {};
        static PieceRange Black(){ return PieceRange(Piece::BLACK_PAWN, Piece::BLACK_KING); }
        static PieceRange White(){ return PieceRange(Piece::WHITE_PAWN, Piece::WHITE_KING); }
        static PieceRange BlackNoKing(){ return PieceRange(Piece::BLACK_PAWN, Piece::BLACK_QUEEN); }
        static PieceRange WhiteNoKing(){ return PieceRange(Piece::WHITE_PAWN, Piece::WHITE_QUEEN); }
        static PieceRange all(){ return PieceRange(Piece::WHITE_PAWN, Piece::BLACK_PAWN); }

    private:
        Piece start;
        Piece stop;
};

class BitBoard {
    private:
        std::array<uint64_t, 12> board;
        uint64_t& operator[](Piece p);
    
    public:
        BitBoard(const std::string& FEN);
        const uint64_t& operator[](Piece p) const;
        void apply_move(const Move& m);
};